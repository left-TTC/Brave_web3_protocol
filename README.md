# README
## *Introduce*
### Modify the Brave browser to support specific domain suffixes
## *Method*
### Lacation: brave source code 
              brave/browser/net/brave_proxying_url_loader_factory.cc
### Function: 
              void BraveProxyingURLLoaderFactory::CreateLoaderAndStart()
### way:
              1.remove the original brave_proxying_url_loader_factory.cc

              2.paste our file
              
              3.rebuild