/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_proxying_url_loader_factory.h"

#include <optional>
#include <string_view>
#include <utility>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/stringprintf.h"
#include "brave/browser/net/brave_request_handler.h"
#include "brave/components/brave_shields/content/browser/adblock_stub_response.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/url_utils.h"
#include "mojo/public/cpp/system/data_pipe_producer.h"
#include "mojo/public/cpp/system/string_data_source.h"
#include "net/base/completion_repeating_callback.h"
#include "net/cookies/site_for_cookies.h"
#include "net/http/http_util.h"
#include "net/url_request/redirect_info.h"
#include "net/url_request/redirect_util.h"
#include "net/url_request/url_request.h"
#include "services/network/public/cpp/parsed_headers.h"
#include "services/network/public/cpp/url_loader_factory_builder.h"
#include "services/network/public/mojom/early_hints.mojom.h"
#include "url/origin.h"
/*this is all my change*/
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "net/http/http_status_code.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/functional/callback.h"
#include "url/gurl.h"
#include "net/base/isolation_info.h"
#include "net/base/network_isolation_key.h"
#include "content/public/browser/storage_partition.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace {

// Helper struct for crafting responses.
struct WriteData {
  base::WeakPtr<network::mojom::URLLoaderClient> client;
  std::string data;
  std::unique_ptr<mojo::DataPipeProducer> producer;
};

void OnWrite(std::unique_ptr<WriteData> write_data, MojoResult result) {
  if (result != MOJO_RESULT_OK || !write_data->client) {
    return;
  }

  network::URLLoaderCompletionStatus status(net::OK);
  status.encoded_data_length = write_data->data.size();
  status.encoded_body_length = write_data->data.size();
  status.decoded_body_length = write_data->data.size();
  write_data->client->OnComplete(status);
}

// Creates simulated net::RedirectInfo when an extension redirects a request,
// behaving like a redirect response was actually returned by the remote server.
net::RedirectInfo CreateRedirectInfo(
    const network::ResourceRequest& original_request,
    const GURL& new_url,
    int response_code,
    const std::optional<std::string>& referrer_policy_header) {
  // Workaround for a bug in Chromium (crbug.com/1097681).
  // download_utils.cc do not set update_first_party_url_on_redirect to true
  // for new ResourceRequests, but we can mitigate it by looking at
  // |is_outermost_main_frame| which is true for navigations and downloads.
  const bool update_first_party_url_on_redirect =
      (original_request.update_first_party_url_on_redirect ||
       original_request.is_outermost_main_frame);
  return net::RedirectInfo::ComputeRedirectInfo(
      original_request.method, original_request.url,
      original_request.site_for_cookies,
      update_first_party_url_on_redirect
          ? net::RedirectInfo::FirstPartyURLPolicy::UPDATE_URL_ON_REDIRECT
          : net::RedirectInfo::FirstPartyURLPolicy::NEVER_CHANGE_URL,
      original_request.referrer_policy, original_request.referrer.spec(),
      response_code, new_url, referrer_policy_header,
      false /* insecure_scheme_was_upgraded */, false /* copy_fragment */,
      false /* is_signed_exchange_fallback_redirect */);
}

}  // namespace

BraveProxyingURLLoaderFactory::InProgressRequest::FollowRedirectParams::
    FollowRedirectParams() = default;
BraveProxyingURLLoaderFactory::InProgressRequest::FollowRedirectParams::
    ~FollowRedirectParams() = default;

BraveProxyingURLLoaderFactory::InProgressRequest::InProgressRequest(
    BraveProxyingURLLoaderFactory& factory,
    uint64_t request_id,
    int32_t network_service_request_id,
    int render_process_id,
    content::FrameTreeNodeId frame_tree_node_id,
    uint32_t options,
    const network::ResourceRequest& request,
    content::BrowserContext* browser_context,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    mojo::PendingReceiver<network::mojom::URLLoader> loader_receiver,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    scoped_refptr<base::SequencedTaskRunner> navigation_response_task_runner)
    : factory_(factory),
      request_(request),
      request_id_(request_id),
      network_service_request_id_(network_service_request_id),
      render_process_id_(render_process_id),
      frame_tree_node_id_(frame_tree_node_id),
      options_(options),
      browser_context_(browser_context),
      traffic_annotation_(traffic_annotation),
      proxied_loader_receiver_(this,
                               std::move(loader_receiver),
                               navigation_response_task_runner),
      target_client_(std::move(client)),
      proxied_client_receiver_(this),
      navigation_response_task_runner_(navigation_response_task_runner),
      weak_factory_(this) {
  // If there is a client error, clean up the request.
  target_client_.set_disconnect_handler(base::BindOnce(
      &BraveProxyingURLLoaderFactory::InProgressRequest::OnRequestError,
      weak_factory_.GetWeakPtr(),
      network::URLLoaderCompletionStatus(net::ERR_ABORTED)));
}

BraveProxyingURLLoaderFactory::InProgressRequest::~InProgressRequest() {
  if (ctx_) {
    factory_->request_handler_->OnURLRequestDestroyed(ctx_);
  }
}

void BraveProxyingURLLoaderFactory::InProgressRequest::Restart() {
  UpdateRequestInfo();
  RestartInternal();
}

void BraveProxyingURLLoaderFactory::InProgressRequest::UpdateRequestInfo() {
  // TODO(iefremov): Update |ctx_| here and get rid of multiple spots where
  // it is refilled.
}

void BraveProxyingURLLoaderFactory::InProgressRequest::RestartInternal() {
  request_completed_ = false;
  start_time_ = base::TimeTicks::Now();

  base::RepeatingCallback<void(int)> continuation =
      base::BindRepeating(&InProgressRequest::ContinueToBeforeSendHeaders,
                          weak_factory_.GetWeakPtr());
  redirect_url_ = GURL();
  ctx_ = brave::BraveRequestInfo::MakeCTX(request_, render_process_id_,
                                          frame_tree_node_id_, request_id_,
                                          browser_context_, ctx_);
  int result = factory_->request_handler_->OnBeforeURLRequest(
      ctx_, continuation, &redirect_url_);

  if (result == net::ERR_BLOCKED_BY_CLIENT) {
    // The request was cancelled synchronously. Dispatch an error notification
    // and terminate the request.
    network::URLLoaderCompletionStatus status(result);
    OnRequestError(status);
    return;
  }

  if (result == net::ERR_IO_PENDING) {
    // One or more listeners is blocking, so the request must be paused until
    // they respond. |continuation| above will be invoked asynchronously to
    // continue or cancel the request.
    //
    // We pause the binding here to prevent further client message processing.
    if (proxied_client_receiver_.is_bound())
      proxied_client_receiver_.Pause();

    return;
  }
  DCHECK_EQ(net::OK, result);

  continuation.Run(net::OK);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::FollowRedirect(
    const std::vector<std::string>& removed_headers,
    const net::HttpRequestHeaders& modified_headers,
    const net::HttpRequestHeaders& modified_cors_exempt_headers,
    const std::optional<GURL>& new_url) {
  if (new_url)
    request_.url = new_url.value();

  for (const std::string& header : removed_headers)
    request_.headers.RemoveHeader(header);
  request_.headers.MergeFrom(modified_headers);

  UpdateRequestInfo();

  if (target_loader_.is_bound()) {
    auto params = std::make_unique<FollowRedirectParams>();
    params->removed_headers = removed_headers;
    params->modified_headers = modified_headers;
    params->modified_cors_exempt_headers = modified_cors_exempt_headers;
    params->new_url = new_url;
    pending_follow_redirect_params_ = std::move(params);
  }

  RestartInternal();
}

void BraveProxyingURLLoaderFactory::InProgressRequest::SetPriority(
    net::RequestPriority priority,
    int32_t intra_priority_value) {
  if (target_loader_.is_bound())
    target_loader_->SetPriority(priority, intra_priority_value);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    PauseReadingBodyFromNet() {
  if (target_loader_.is_bound())
    target_loader_->PauseReadingBodyFromNet();
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    ResumeReadingBodyFromNet() {
  if (target_loader_.is_bound())
    target_loader_->ResumeReadingBodyFromNet();
}

void BraveProxyingURLLoaderFactory::InProgressRequest::OnReceiveEarlyHints(
    network::mojom::EarlyHintsPtr early_hints) {}

void BraveProxyingURLLoaderFactory::InProgressRequest::OnReceiveResponse(
    network::mojom::URLResponseHeadPtr head,
    mojo::ScopedDataPipeConsumerHandle body,
    std::optional<mojo_base::BigBuffer> cached_metadata) {
  current_response_head_ = std::move(head);
  current_response_body_ = std::move(body);
  cached_metadata_ = std::move(cached_metadata);
  ctx_->internal_redirect = false;
  HandleResponseOrRedirectHeaders(
      base::BindRepeating(&InProgressRequest::ContinueToResponseStarted,
                          weak_factory_.GetWeakPtr()));
}

void BraveProxyingURLLoaderFactory::InProgressRequest::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    network::mojom::URLResponseHeadPtr head) {
  current_response_head_ = std::move(head);
  DCHECK(ctx_);
  ctx_->internal_redirect = false;
  HandleResponseOrRedirectHeaders(
      base::BindRepeating(&InProgressRequest::ContinueToBeforeRedirect,
                          weak_factory_.GetWeakPtr(), redirect_info));
}

void BraveProxyingURLLoaderFactory::InProgressRequest::OnUploadProgress(
    int64_t current_position,
    int64_t total_size,
    OnUploadProgressCallback callback) {
  target_client_->OnUploadProgress(current_position, total_size,
                                   std::move(callback));
}

void BraveProxyingURLLoaderFactory::InProgressRequest::OnTransferSizeUpdated(
    int32_t transfer_size_diff) {
  target_client_->OnTransferSizeUpdated(transfer_size_diff);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::OnComplete(
    const network::URLLoaderCompletionStatus& status) {
  UMA_HISTOGRAM_TIMES("Brave.ProxyingURLLoader.TotalRequestTime",
                      base::TimeTicks::Now() - start_time_);
  if (status.error_code != net::OK) {
    OnRequestError(status);
    return;
  }
  target_client_->OnComplete(status);

  // Deletes |this|.
  factory_->RemoveRequest(this);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    HandleBeforeRequestRedirect() {
  // The listener requested a redirect. Close the connection with the current
  // URLLoader and inform the URLLoaderClient redirect was generated.
  // To load |redirect_url_|, a new URLLoader will be recreated
  // after receiving FollowRedirect().

  // Forgetting to close the connection with the current URLLoader caused
  // bugs. The latter doesn't know anything about the redirect. Continuing
  // the load with it gives unexpected results. See
  // https://crbug.com/882661#c72.
  proxied_client_receiver_.reset();
  target_loader_.reset();

  constexpr int kInternalRedirectStatusCode = 307;

  net::RedirectInfo redirect_info =
      CreateRedirectInfo(request_, redirect_url_, kInternalRedirectStatusCode,
                         std::nullopt /* referrer_policy_header */);

  network::mojom::URLResponseHeadPtr head =
      network::mojom::URLResponseHead::New();
  std::string headers = base::StringPrintf(
      "HTTP/1.1 %i Internal Redirect\n"
      "Location: %s\n"
      "Non-Authoritative-Reason: WebRequest API\n\n",
      kInternalRedirectStatusCode, redirect_url_.spec().c_str());

  // Cross-origin requests need to modify the Origin header to 'null'. Since
  // CorsURLLoader sets |request_initiator| to the Origin request header in
  // NetworkService, we need to modify |request_initiator| here to craft the
  // Origin header indirectly.
  // Following checks implement the step 10 of "4.4. HTTP-redirect fetch",
  // https://fetch.spec.whatwg.org/#http-redirect-fetch
  if (request_.request_initiator &&
      (!url::Origin::Create(redirect_url_)
            .IsSameOriginWith(url::Origin::Create(request_.url)) &&
       !request_.request_initiator->IsSameOriginWith(
           url::Origin::Create(request_.url)))) {
    // Reset the initiator to pretend tainted origin flag of the spec is set.
    request_.request_initiator = url::Origin();
  }
  head->headers = base::MakeRefCounted<net::HttpResponseHeaders>(
      net::HttpUtil::AssembleRawHeaders(headers));
  head->encoded_data_length = 0;

  current_response_head_ = std::move(head);
  ctx_->internal_redirect = true;
  ContinueToBeforeRedirect(redirect_info, net::OK);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    ContinueToBeforeSendHeaders(int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  if (!redirect_url_.is_empty()) {
    HandleBeforeRequestRedirect();
    return;
  }

  DCHECK(ctx_);
  if (ctx_->new_referrer.has_value()) {
    request_.referrer = ctx_->new_referrer.value();
  }

  if (proxied_client_receiver_.is_bound())
    proxied_client_receiver_.Resume();

  // TODO(iefremov): Shorten
  if (ctx_->blocked_by != brave::kNotBlocked) {
    if (!ctx_->ShouldMockRequest()) {
      OnRequestError(
          network::URLLoaderCompletionStatus(net::ERR_BLOCKED_BY_CLIENT));
      return;
    }

    auto response = network::mojom::URLResponseHead::New();
    std::string response_data;
    brave_shields::MakeStubResponse(ctx_->mock_data_url, request_, &response,
                                    &response_data);

    // Create a data pipe for transmitting the response.
    mojo::ScopedDataPipeProducerHandle producer;
    mojo::ScopedDataPipeConsumerHandle consumer;
    if (CreateDataPipe(nullptr, producer, consumer) != MOJO_RESULT_OK) {
      OnRequestError(
          network::URLLoaderCompletionStatus(net::ERR_INSUFFICIENT_RESOURCES));
      return;
    }

    // Craft the response.
    target_client_->OnReceiveResponse(std::move(response), std::move(consumer),
                                      std::move(cached_metadata_));

    auto write_data = std::make_unique<WriteData>();
    write_data->client = weak_factory_.GetWeakPtr();
    write_data->data = response_data;
    write_data->producer =
        std::make_unique<mojo::DataPipeProducer>(std::move(producer));

    std::string_view string_piece(write_data->data);
    WriteData* write_data_ptr = write_data.get();
    write_data_ptr->producer->Write(
        std::make_unique<mojo::StringDataSource>(
            string_piece, mojo::StringDataSource::AsyncWritingMode::
                              STRING_STAYS_VALID_UNTIL_COMPLETION),
        base::BindOnce(OnWrite, std::move(write_data)));
    return;
  }

  if (request_.url.SchemeIsHTTPOrHTTPS()) {
    auto continuation = base::BindRepeating(
        &InProgressRequest::ContinueToSendHeaders, weak_factory_.GetWeakPtr());

    ctx_ = brave::BraveRequestInfo::MakeCTX(request_, render_process_id_,
                                            frame_tree_node_id_, request_id_,
                                            browser_context_, ctx_);
    int result = factory_->request_handler_->OnBeforeStartTransaction(
        ctx_, continuation, &request_.headers);

    if (result == net::ERR_BLOCKED_BY_CLIENT) {
      // The request was cancelled synchronously. Dispatch an error notification
      // and terminate the request.
      OnRequestError(network::URLLoaderCompletionStatus(result));
      return;
    }

    if (result == net::ERR_IO_PENDING) {
      // One or more listeners is blocking, so the request must be paused until
      // they respond. |continuation| above will be invoked asynchronously to
      // continue or cancel the request.
      //
      // We pause the binding here to prevent further client message processing.
      if (proxied_client_receiver_.is_bound())
        proxied_client_receiver_.Pause();
      return;
    }
    DCHECK_EQ(net::OK, result);
  }

  ContinueToSendHeaders(net::OK);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::ContinueToStartRequest(
    int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  if (proxied_client_receiver_.is_bound())
    proxied_client_receiver_.Resume();

  if (!target_loader_.is_bound() && factory_->target_factory_.is_bound()) {
    // Nothing has cancelled us up to this point, so it's now OK to
    // initiate the real network request.
    uint32_t options = options_;
    factory_->target_factory_->CreateLoaderAndStart(
        target_loader_.BindNewPipeAndPassReceiver(
            navigation_response_task_runner_),
        network_service_request_id_, options, request_,
        proxied_client_receiver_.BindNewPipeAndPassRemote(
            navigation_response_task_runner_),
        traffic_annotation_);
  }

  // From here the lifecycle of this request is driven by subsequent events on
  // either |proxied_loader_receiver_|, |proxied_client_receiver_|.
}

void BraveProxyingURLLoaderFactory::InProgressRequest::ContinueToSendHeaders(
    int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }
  const std::set<std::string>& removed_headers = ctx_->removed_headers;
  const std::set<std::string>& set_headers = ctx_->set_headers;

  if (pending_follow_redirect_params_) {
    pending_follow_redirect_params_->removed_headers.insert(
        pending_follow_redirect_params_->removed_headers.end(),
        removed_headers.begin(), removed_headers.end());

    for (auto& set_header : set_headers) {
      std::optional<std::string> header_value =
          request_.headers.GetHeader(set_header);
      if (header_value) {
        pending_follow_redirect_params_->modified_headers.SetHeader(
            set_header, *header_value);
      } else {
        NOTREACHED_IN_MIGRATION();
      }
    }

    if (target_loader_.is_bound()) {
      target_loader_->FollowRedirect(
          pending_follow_redirect_params_->removed_headers,
          pending_follow_redirect_params_->modified_headers,
          pending_follow_redirect_params_->modified_cors_exempt_headers,
          pending_follow_redirect_params_->new_url);
    }

    pending_follow_redirect_params_.reset();
  }

  if (proxied_client_receiver_.is_bound())
    proxied_client_receiver_.Resume();
  ContinueToStartRequest(net::OK);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    ContinueToResponseStarted(int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  if (override_headers_) {
    current_response_head_->headers = override_headers_;
    // Since we overrode headers we should reparse them:
    // NavigationRequest::ComputePoliciesToCommit uses parsed headers to set
    // CSP, so if we don't reparse our CSP header changes won't work.
    current_response_head_->parsed_headers = network::PopulateParsedHeaders(
        current_response_head_->headers.get(), request_.url);
  }

  std::string redirect_location;
  if (override_headers_ && override_headers_->IsRedirect(&redirect_location)) {
    // The response headers may have been overridden by an |onHeadersReceived|
    // handler and may have been changed to a redirect. We handle that here
    // instead of acting like regular request completion.
    //
    // Note that we can't actually change how the Network Service handles the
    // original request at this point, so our "redirect" is really just
    // generating an artificial |onBeforeRedirect| event and starting a new
    // request to the Network Service. Our client shouldn't know the difference.
    GURL new_url(redirect_location);

    net::RedirectInfo redirect_info = CreateRedirectInfo(
         request_, new_url, override_headers_->response_code(),
         net::RedirectUtil::GetReferrerPolicyHeader(override_headers_.get()));

    // These will get re-bound if a new request is initiated by
    // |FollowRedirect()|.
    proxied_client_receiver_.reset();
    target_loader_.reset();

    ctx_->internal_redirect = true;
    ContinueToBeforeRedirect(redirect_info, net::OK);
    return;
  }

  proxied_client_receiver_.Resume();
  target_client_->OnReceiveResponse(std::move(current_response_head_),
                                    std::move(current_response_body_),
                                    std::move(cached_metadata_));
}

void BraveProxyingURLLoaderFactory::InProgressRequest::ContinueToBeforeRedirect(
    const net::RedirectInfo& redirect_info,
    int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  if (proxied_client_receiver_.is_bound())
    proxied_client_receiver_.Resume();

  if (ctx_->internal_redirect) {
    ctx_->redirect_source = GURL();
  } else {
    ctx_->redirect_source = request_.url;
  }
  target_client_->OnReceiveRedirect(redirect_info,
                                    std::move(current_response_head_));
  request_.url = redirect_info.new_url;
  request_.method = redirect_info.new_method;
  request_.site_for_cookies = redirect_info.new_site_for_cookies;
  request_.referrer = GURL(redirect_info.new_referrer);
  request_.referrer_policy = redirect_info.new_referrer_policy;

  if (request_.trusted_params) {
    request_.trusted_params->isolation_info =
        request_.trusted_params->isolation_info.CreateForRedirect(
            url::Origin::Create(redirect_info.new_url));
  }

  // The request method can be changed to "GET". In this case we need to
  // reset the request body manually.
  if (request_.method == net::HttpRequestHeaders::kGetMethod)
    request_.request_body = nullptr;

  request_completed_ = true;
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    HandleResponseOrRedirectHeaders(net::CompletionOnceCallback continuation) {
  override_headers_ = nullptr;
  redirect_url_ = GURL();

  auto split_once_callback = base::SplitOnceCallback(std::move(continuation));
  if (request_.url.SchemeIsHTTPOrHTTPS()) {
    ctx_ = brave::BraveRequestInfo::MakeCTX(request_, render_process_id_,
                                            frame_tree_node_id_, request_id_,
                                            browser_context_, ctx_);
    int result = factory_->request_handler_->OnHeadersReceived(
        ctx_, std::move(split_once_callback.first),
        current_response_head_->headers.get(), &override_headers_,
        &redirect_url_);

    if (result == net::ERR_BLOCKED_BY_CLIENT) {
      OnRequestError(network::URLLoaderCompletionStatus(result));
      return;
    }

    if (result == net::ERR_IO_PENDING) {
      // One or more listeners is blocking, so the request must be paused until
      // they respond. |continuation| above will be invoked asynchronously to
      // continue or cancel the request.
      //
      // We pause the binding here to prevent further client message processing.
      proxied_client_receiver_.Pause();
      return;
    }

    DCHECK_EQ(net::OK, result);
  }

  std::move(split_once_callback.second).Run(net::OK);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::OnRequestError(
    const network::URLLoaderCompletionStatus& status) {
  if (!request_completed_) {
    // Make a non-const copy of status so that |should_collapse_initiator| can
    // be modified
    network::URLLoaderCompletionStatus collapse_status(status);

    if (base::FeatureList::IsEnabled(
            ::brave_shields::features::kBraveAdblockCollapseBlockedElements) &&
        ctx_->blocked_by == brave::kAdBlocked) {
      collapse_status.should_collapse_initiator = true;
    }

    target_client_->OnComplete(collapse_status);
  }

  // Deletes |this|.
  factory_->RemoveRequest(this);
}

BraveProxyingURLLoaderFactory::BraveProxyingURLLoaderFactory(
    BraveRequestHandler& request_handler,
    content::BrowserContext* browser_context,
    int render_process_id,
    content::FrameTreeNodeId frame_tree_node_id,
    network::URLLoaderFactoryBuilder& factory_builder,
    scoped_refptr<RequestIDGenerator> request_id_generator,
    DisconnectCallback on_disconnect,
    scoped_refptr<base::SequencedTaskRunner> navigation_response_task_runner)
    : request_handler_(request_handler),
      browser_context_(browser_context),
      render_process_id_(render_process_id),
      frame_tree_node_id_(frame_tree_node_id),
      request_id_generator_(request_id_generator),
      disconnect_callback_(std::move(on_disconnect)),
      navigation_response_task_runner_(
          std::move(navigation_response_task_runner)),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(proxy_receivers_.empty());
  DCHECK(!target_factory_.is_bound());

  auto [receiver, target_factory] = factory_builder.Append();

  target_factory_.Bind(std::move(target_factory));
  target_factory_.set_disconnect_handler(
      base::BindOnce(&BraveProxyingURLLoaderFactory::OnTargetFactoryError,
                     base::Unretained(this)));

  proxy_receivers_.Add(this, std::move(receiver),
                       navigation_response_task_runner_);
  proxy_receivers_.set_disconnect_handler(
      base::BindRepeating(&BraveProxyingURLLoaderFactory::OnProxyBindingError,
                          base::Unretained(this)));
}

BraveProxyingURLLoaderFactory::~BraveProxyingURLLoaderFactory() = default;

// static
void BraveProxyingURLLoaderFactory::MaybeProxyRequest(
    content::BrowserContext* browser_context,
    content::RenderFrameHost* render_frame_host,
    int render_process_id,
    network::URLLoaderFactoryBuilder& factory_builder,
    scoped_refptr<base::SequencedTaskRunner> navigation_response_task_runner) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  ResourceContextData::StartProxying(
      browser_context, render_process_id,
      render_frame_host ? render_frame_host->GetFrameTreeNodeId()
                        : content::FrameTreeNodeId(),
      factory_builder, navigation_response_task_runner);
}

// if contains "web3"
bool if_need_handle_url(const GURL& input_url) {
  std::string domain = input_url.host();
  const std::string web3_suffix = ".web3";
  if (domain.length() >= web3_suffix.length()) {
    return domain.compare(
        domain.length() - web3_suffix.length(), 
        web3_suffix.length(),                   
        web3_suffix                             
    ) == 0;
  }
  return false;
}

net::NetworkTrafficAnnotationTag Ktraffic_annotation =
    net::DefineNetworkTrafficAnnotation("brave_sync_fetch", R"(
      semantics {
        sender: "Brave Sync Fetch"
        description: "Fetch data from localhost"
        trigger: "User initiated"
        data: "No user data"
        destination: LOCALHOST
      }
      policy {
        cookies_allowed: NO
        setting: "This feature cannot be disabled"
        policy_exception_justification: "Not implemented"
      })");

//this is a temporlate test function
//curl solana API and get website's cid
void curlSolana(
    const GURL& url,
    network::mojom::URLLoaderFactory* url_loader_factory,
    base::OnceCallback<void(const std::string&)> callback) {
  if (!url_loader_factory || !url.is_valid()) {
    std::move(callback).Run("");
    return;
  }

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GURL("https://www.baidu.com");
  resource_request->method = "GET";
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;

  std::unique_ptr<network::SimpleURLLoader> loader =
      network::SimpleURLLoader::Create(std::move(resource_request),
                                      Ktraffic_annotation);

  auto* loader_ptr = loader.get();
  loader_ptr->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory,
      base::BindOnce(
          [](std::unique_ptr<network::SimpleURLLoader> loader,
             base::OnceCallback<void(const std::string&)> callback,
             std::unique_ptr<std::string> response) {
            std::string content = response ? *response : "";
            std::move(callback).Run(content);
          },
          std::move(loader), std::move(callback)));
          LOG(INFO) << "我们进行了一次cid查询";
}

void get_new_url(const GURL& input_url,
                 network::mojom::URLLoaderFactory* factory,
                 base::OnceCallback<void(const GURL&)> callback) {
  const std::string port = "8080";
  const std::string scheme = "http";
  const std::string host = "127.0.0.1";
  const std::string path_one = "/ipfs/";
  const std::string path_origin = input_url.path();

  curlSolana(
      input_url, factory,
      base::BindOnce(
          [](const std::string& path_origin, const std::string& scheme,
             const std::string& host, const std::string& port,
             const std::string& path_one,
             base::OnceCallback<void(const GURL&)> callback,
             const std::string& content) {
              LOG(INFO) << "我们成功的查询到了一次CID:"<< content;
              std::string cid = content;
              if (!cid.empty() && cid.back() == '\n') {
                cid.pop_back();
              }
              std::string path;
              std::string last_need_add;

              if (path_origin.empty() || path_origin == "/") {
                last_need_add = "/index.html";
              } else {
                last_need_add = path_origin;
              }
              path = path_one + cid + last_need_add;

              GURL new_url = GURL(scheme + "://" + host + ":" + port + path);
              std::move(callback).Run(new_url);
            },
          path_origin, scheme, host, port, path_one, std::move(callback)));
}

// check the request type
net::IsolationInfo::RequestType choose_request_type(const GURL& input_referrer) {
  if (input_referrer.is_empty()) {
    return net::IsolationInfo::RequestType::kMainFrame;
  } else {
    return net::IsolationInfo::RequestType::kOther;
  }
}

void BraveProxyingURLLoaderFactory::CreateLoaderAndStart(
    mojo::PendingReceiver<network::mojom::URLLoader> loader_receiver,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  LOG(INFO) << "进入了brave拦截函数";
  const GURL& check_url = request.url;
  //First check if you need to change the url
  if (if_need_handle_url(check_url)) {
    network::ResourceRequest modified_request = request;
    //next get the url_factory
    auto* storage_partition = browser_context_->GetDefaultStoragePartition();
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory =
        storage_partition->GetURLLoaderFactoryForBrowserProcess();

    get_new_url(
        check_url, url_loader_factory.get(),
        base::BindOnce(
            [](BraveProxyingURLLoaderFactory* factory,
               network::ResourceRequest modified_request,
               mojo::PendingReceiver<network::mojom::URLLoader> loader_receiver,
               int32_t request_id, uint32_t options,
               mojo::PendingRemote<network::mojom::URLLoaderClient> client,
               const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
               const GURL& new_url) {
                //check referrer
                net::IsolationInfo::RequestType request_type =
                    choose_request_type(modified_request.referrer);
                //get a new url
                modified_request.url = new_url;
                //create new site_for_cookie for mainFrame style
                if (request_type == net::IsolationInfo::RequestType::kMainFrame) {
                  if (!modified_request.trusted_params) {
                    modified_request.trusted_params.emplace();
                  }
                  modified_request.site_for_cookies =
                      net::SiteForCookies::FromOrigin(url::Origin::Create(new_url));
                  modified_request.trusted_params->isolation_info =
                      net::IsolationInfo::Create(
                          request_type,
                          url::Origin::Create(new_url),  // top_frame_origin
                          url::Origin::Create(new_url),  // frame_origin
                          modified_request.site_for_cookies);
                }

                const uint64_t brave_request_id = factory->request_id_generator_->Generate();
                auto result = factory->requests_.emplace(
                    std::make_unique<InProgressRequest>(
                        *factory, brave_request_id, request_id,
                        factory->render_process_id_, factory->frame_tree_node_id_,
                        options, modified_request, factory->browser_context_,
                        traffic_annotation, std::move(loader_receiver),
                        std::move(client),
                        factory->navigation_response_task_runner_));
                (*result.first)->Restart();
                LOG(INFO) << "web3域名开始了";
            },
            this, modified_request, std::move(loader_receiver), request_id,
            options, std::move(client), traffic_annotation));
            LOG(INFO) << "web3离开了brave拦截函数";
  } else {
    // The request ID doesn't really matter in the Network Service path. It just
    // needs to be unique per-BrowserContext so request handlers can make sense of
    // it. Note that |network_service_request_id_| by contrast is not necessarily
    // unique, so we don't use it for identity here.
    const uint64_t brave_request_id = request_id_generator_->Generate();
    auto result = requests_.emplace(std::make_unique<InProgressRequest>(
        *this, brave_request_id, request_id, render_process_id_,
        frame_tree_node_id_, options, request, browser_context_,
        traffic_annotation, std::move(loader_receiver), std::move(client),
        navigation_response_task_runner_));
    (*result.first)->Restart();
    LOG(INFO) << "离开了brave拦截函数";
  }
}

void BraveProxyingURLLoaderFactory::Clone(
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> loader_receiver) {
  proxy_receivers_.Add(this, std::move(loader_receiver));
}

void BraveProxyingURLLoaderFactory::OnTargetFactoryError() {
  // Stop calls to CreateLoaderAndStart() when |target_factory_| is invalid.
  target_factory_.reset();
  proxy_receivers_.Clear();
  MaybeRemoveProxy();
}

void BraveProxyingURLLoaderFactory::OnProxyBindingError() {
  if (proxy_receivers_.empty())
    target_factory_.reset();

  MaybeRemoveProxy();
}

void BraveProxyingURLLoaderFactory::RemoveRequest(InProgressRequest* request) {
  auto it = requests_.find(request);
  DCHECK(it != requests_.end());
  requests_.erase(it);

  MaybeRemoveProxy();
}

void BraveProxyingURLLoaderFactory::MaybeRemoveProxy() {
  // Even if all URLLoaderFactory pipes connected to this object have been
  // closed it has to stay alive until all active requests have completed.
  if (target_factory_.is_bound() || !requests_.empty())
    return;

  // Deletes |this|.
  std::move(disconnect_callback_).Run(this);
}