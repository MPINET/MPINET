# Routing Schemes

## UGAL Routing (IPv4)

### Usage and Features

* adding an Ipv4UGALRoutingHelper to the ListRoutingHelper during installing the internet stack
* installing the congestion monitor application on all switch nodes
* UGAL routing only works on unicast flows, so you may need an additional routing protocol for other flows (i.e. a global routing protocol), adding them below the UGAL routing helper
* either set respond to link changes to true or call the function NotifyLinkChanges() manually after reconfiguration
* enable flow routing may cause infinite loops in the network, since link changes will only flush the routing table of the directly connected nodes