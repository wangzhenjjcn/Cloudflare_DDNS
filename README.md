# Cloudflare_DDNS
DDNS for cloudflare

##  See branch windowsx64

##  **Usage**

modify bat file as:

`cloudflare.sh -k cloudflareApiKey -u cloudflareUsername -z domainName -h ddnsDomainName -t AAAA -f true -w "http://ipv4.icanhazip.com"`

```bash
# cloudflare.sh -k cloudflare-api-key \
#            	-u user@example.com \
#            	-h host.example.com \     # fqdn of the record you want to update
#            	-z example.com \          # will show you all zones if forgot, but you need this
#            	-t A|AAAA                 # specify ipv4/ipv6, default: ipv4
#            	-f false|true \           # force dns update, disregard local stored ip
#            	-w ipCheck website \      # "http://ipv4.icanhazip.com"
```
