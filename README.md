# DNS-Holdon/Dadder

A DNS forwarder which implement the following paper: 

**[Hold-On: Protecting Against On-Path DNS Poisoning, Securing and Trusting Internet Names, SATIN 2012](https://www1.icsi.berkeley.edu/~nweaver/papers/2012-satin-holdon.pdf)**

**Dadder**(DNS-Holdon) operates as a stub resolver to a known-uncensored remote recursive resolver. As several attacks on DNS inject forged DNS replies without suppressing the legitimate replies. Current implementations of DNS resolvers are vulnerable to accepting the injected replies if the attacker’s reply arrives before the legitimate one. In the case of regular DNS, this behavior allows an attacker to corrupt a victim’s interpretation of a name.


The DNS-Holdon will wait after receiving an initial reply for a “Hold-On” period to allow a subsequent legitimate reply to also arrive, and validates DNS replies with the IP TTL and the timing of the replies. As a prototype, it functions without perceptible performance decrease for undisrupted lookups.


For Linux or Mac OS only.

## Usage

```
make;
./dadder -c dadder.conf
```

## Dadder(DNS-Holdon) for Openwrt

```Bash
  pushd package
  #git clone https://github.com/gr1x/dns-holdon-openwrt.git dns-holdon
  git clone git@github.com:gr1x/dns-holdon-openwrt.git dns-holdon
  popd
  # select Network/Holdon-DNS
  # Make sure "CONFIG_PACKAGE_dns-holdon=m" is within the .config file.
  make menuconfig
  make -j16 V=s
```
### Install


**Only tested on Lenove Y1**


As Lenovo Y1 is a small sized router with SOC MediaTek_MT7620a, and according to OpenWrt specifics that all MediaTek/Ralink SoCs are merged under the target ramips, so the first thing to do is to download the right SDK
from https://downloads.openwrt.org/, or one can compile SDK from scratch by oneself.


Lenovo Y1's OpenWRT SDK : https://downloads.openwrt.org/chaos_calmer/15.05/ramips/mt7620/OpenWrt-SDK-15.05-ramips-mt7620_gcc-4.8-linaro_uClibc-0.9.33.2.Linux-x86_64.tar.bz2


`tar xjf` the SDK, ```cd``` into the SDK and run the following cmd:

```Bash
  pushd package
  #git clone https://github.com/gr1x/dns-holdon-openwrt.git dns-holdon
  git clone git@github.com:gr1x/dns-holdon-openwrt.git dns-holdon
  popd
  # select Network/Holdon-DNS
  # Make sure "CONFIG_PACKAGE_dns-holdon=m" is within the .config file.
  make menuconfig
  make -j16 V=s
```

When no errors, find the ipk package bin/ramips/packages/base/dns-holdon_1-1_ramips_24kec.ipk and scp it to your router.

```Bash
root@OpenWrt:~# opkg install *.ipk
```

Change the following config files at your will:
- /etc/dadder.config
- /etc/badip.txt
- /etc/blacklist.txt

### Port
```Bash
root@OpenWrt:~# netstat -ant
Active Internet connections (servers and established)
Proto Recv-Q Send-Q Local Address           Foreign Address         State
tcp        0      0 0.0.0.0:5335            0.0.0.0:*               LISTEN

root@OpenWrt:~# netstat -anu
Active Internet connections (servers and established)
Proto Recv-Q Send-Q Local Address           Foreign Address         State
udp        0      0 0.0.0.0:53              0.0.0.0:*
udp        0      0 0.0.0.0:5335            0.0.0.0:*
udp        0      0 :::53                   :::*
```

### Working Principle


**Client**

```Bash
dig @192.168.1.1 -p 5335 www.youtube.com

; <<>> DiG 9.9.5 <<>> @192.168.1.1 -p 5335 www.youtube.com; (1 server found)
;; global options: +cmd
;; Got answer:
;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 53711
;; flags: qr rd ra; QUERY: 1, ANSWER: 8, AUTHORITY: 0, ADDITIONAL: 1

;; OPT PSEUDOSECTION:
; EDNS: version: 0, flags:; udp: 512
;; QUESTION SECTION:
;www.youtube.com.               IN      A

;; ANSWER SECTION:
www.youtube.com.        21599   IN      CNAME   youtube-ui.l.google.com.
youtube-ui.l.google.com. 899    IN      CNAME   youtube-ui-china.l.google.com.
youtube-ui-china.l.google.com. 179 IN   A       74.125.203.138
youtube-ui-china.l.google.com. 179 IN   A       74.125.203.102
youtube-ui-china.l.google.com. 179 IN   A       74.125.203.101
youtube-ui-china.l.google.com. 179 IN   A       74.125.203.100
youtube-ui-china.l.google.com. 179 IN   A       74.125.203.113
youtube-ui-china.l.google.com. 179 IN   A       74.125.203.139

;; Query time: 82 msec
;; SERVER: 192.168.1.1#5335(192.168.1.1)
;; WHEN: Sat Jan 30 12:56:52 CST 2016
;; MSG SIZE  rcvd: 205
```

**Router**
```Bash
root@OpenWrt:/tmp# dadder -c /etc/dadder.config
2016-01-30 04-56-33 M: M_TTL: 40 147
2016-01-30 04-56-52 Q: from: 192.168.1.30:39607 name:www.youtube.com id:53711 len:44
2016-01-30 04-56-52 F: to: 8.8.8.8 name: www.youtube.com id: 18979
2016-01-30 04-56-52 R: from:8.8.8.8 len:49 ttl:99
2016-01-30 04-56-52 D: expect goodTTL[0]=40, goodTTL[1]=147, threshold:3, to validate:99
2016-01-30 04-56-52 I: TTL mismatch, expect: 40 got: 99
2016-01-30 04-56-52 I: BADIP: name:www.youtube.com IP: 203.98.7.65
2016-01-30 04-56-52 I: got_badip_from_open: Name: www.youtube.com IP: 203.98.7.65
2016-01-30 04-56-52 R: from:8.8.8.8 len:64 ttl:51
2016-01-30 04-56-52 D: expect goodTTL[0]=40, goodTTL[1]=147, threshold:3, to validate:51
2016-01-30 04-56-52 I: TTL mismatch, expect: 40 got: 51
2016-01-30 04-56-52 I: BADIP: name:www.youtube.com IP: 93.46.8.89
2016-01-30 04-56-52 I: got_badip_from_open: Name: www.youtube.com IP: 93.46.8.89
2016-01-30 04-56-52 R: from:8.8.8.8 len:205 ttl:38
2016-01-30 04-56-52 D: expect goodTTL[0]=40, goodTTL[1]=147, threshold:3, to validate:38
2016-01-30 04-56-52 F: to_client: 192.168.1.30 name: www.youtube.com,  answer_ip: , responseLen:205
```
