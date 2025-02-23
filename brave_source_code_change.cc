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
  resource_request->url = GURL("http://127.0.0.1");
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
            },
            this, modified_request, std::move(loader_receiver), request_id,
            options, std::move(client), traffic_annotation));
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
  }
}