@echo off
:start

cloudflare.sh -k cloudflareApiKey -u cloudflareUsername -z domainName -h ddnsDomainName -t A
cloudflare.sh -k cloudflareApiKey -u cloudflareUsername -z domainName -h ddnsDomainName -t AAAA

cloudflare.sh -k cloudflareApiKey -u cloudflareUsername -z domainName -h ddnsDomainName -t A -f true -w "http://ipv4.icanhazip.com"
cloudflare.sh -k cloudflareApiKey -u cloudflareUsername -z domainName -h ddnsDomainName -t AAAA -f true -w "http://ipv4.icanhazip.com"

echo %date%%time% UPDATED

ping -n 60 127.0.0.1 >nul 2>nul
ping -n 600 127.0.0.1 >nul 2>nul
ping -n 6000 127.0.0.1 >nul 2>nul

goto start

