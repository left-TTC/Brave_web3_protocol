# README
## Introduce
> Modify the Brave browser to support specific domain suffixes
## Method
### Lacation: brave source code 
```
brave/browser/net/brave_proxying_url_loader_factory.cc
```
### Function: 
``` cpp
void BraveProxyingURLLoaderFactory::CreateLoaderAndStart()
```
### way:
1.remove the original file
```
brave_proxying_url_loader_factory.cc
```
2.paste our file

3.rebuild


## Questions 

> How to use domain name

### The Frist method:(such as domain.web3)
```
https://domain.web3
```
* This is the well-known and recommended way, and after the frist input the extire domain. You can use domain.web3 to navigate on the same device in the future

### The Second method:
```
domain.web3
```
* If you have used the frist method before, This approach is entirely feasible, but if you use this method to navigate before initializing with the https method, although you can navigate, the effect is unsatisfactory and the browser will not remember it.
