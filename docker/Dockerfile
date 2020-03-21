FROM centos:8

# Installation of squidtube's runtime and build dependencies
RUN yum -y install \
        epel-release \
        qt5-qtbase \
        qt5-qtdeclarative \
        sqlite \
        squid \
    && \
    yum -y install \
        ansible \
        git \
        gcc-c++ \
        make \
        qt5-qtbase-devel \
        qt5-qtdeclarative-devel

# Configuration of environment variables
ENV squidUser=squid \
    squidGroup=squid \
    squidCA=/etc/squid/ca-SslPeekAndSplice \
    squidConf=/etc/squid/squid.conf \
    squidHelpers=/usr/lib64/squid \
    squidData=/var/spool/squid

ENV squidtubeBin=/usr/local/bin \
    squidtubeConf=/usr/local/etc/squidtube \
    squidtubeShare=/usr/local/share/squidtube \
    squidtubeData=/var/lib/squidtube

# squidtube installation from source
RUN cd /usr/local/src && \
    git clone https://github.com/amg1127/squidtube && \
    cd squidtube && \
    qmake-qt5 squidtube.pro 'CONFIG+=debug' 'install_bin_dir="'${squidtubeBin}'"' 'install_etc_dir="'${squidtubeConf}'"' 'install_share_dir="'${squidtubeShare}'"' && \
    make -j `grep -wc processor /proc/cpuinfo` && \
    make -j install

# Configuration of a persistent AUFS cache directory for squid
RUN ansible localhost -m file -a 'path="'${squidData}'/cache" state=directory owner="'${squidUser}'" group="'${squidGroup}'" mode="0700"' && \
    ansible localhost -m file -a 'path="/run/squid.pid" state=touch owner="'${squidUser}'" group="'${squidGroup}'" mode="0600"' && \
    ansible localhost -m replace -a 'path="'${squidConf}'" regexp="^[\s#]*cache_dir\s+.*$" replace="cache_dir aufs '${squidData}'/cache 500 16 256"' && \
    su -s /bin/bash -l -c '/usr/sbin/squid -N -z -F' squid

# Setup of a persistent database for squidtube
RUN ansible localhost -m file -a 'path="'${squidtubeData}'" state=directory owner="'${squidUser}'" group="'${squidGroup}'" mode="0700"' && \
    ansible localhost -m ini_file -a 'path="'${squidtubeConf}'/squidtube.conf" section=db option=driver value=QSQLITE' && \
    ansible localhost -m ini_file -a 'path="'${squidtubeConf}'/squidtube.conf" section=db option=name value="'${squidtubeData}'/data.sqlite"'

# Deployment of a CA certificate for Squid HTTPS traffic decryption and SslPeekAndSplice configuration
RUN ansible localhost -m openssl_privatekey -a 'path="'${squidCA}'.key" group="'${squidGroup}'" mode="0640"' && \
    ansible localhost -m openssl_csr -a 'path="'${squidCA}'.csr" privatekey_path="'${squidCA}'.key" CN="Squid CA" basic_constraints="CA:TRUE" basic_constraints_critical=yes' && \
    ansible localhost -m openssl_certificate -a 'path="'${squidCA}'.crt" provider=selfsigned privatekey_path="'${squidCA}'.key" csr_path="'${squidCA}'.csr" mode="0644"' && \
    ansible localhost -m blockinfile -a 'path="'${squidConf}'" marker="# {mark} URL lists for SslPeekAndSplice and squidtube filtering" insertbefore=BOF block="\
acl squidtubeURLs ssl::server_name_regex \\.youtu\\.be$ \n\
acl squidtubeURLs ssl::server_name_regex \\.youtube\\.com$ \n\
acl squidtubeURLs ssl::server_name_regex \\.youtube\\.com\\.[a-z][a-z]$ \n\
    "' && \
    ansible localhost -m replace -a 'path="'${squidConf}'" regexp="^([\s#]*http_port\s+\d+.*)$" replace="# \1 # SslPeekAndSplice configuration"' && \
    ansible localhost -m blockinfile -a 'path="'${squidConf}'" marker="# {mark} SslPeekAndSplice configuration block" insertafter="(?i)^[\s#]*http_port\s+\d+.*# SslPeekAndSplice\s+configuration(\s.*)?$" block="\
http_port 3128 ssl-bump generate-host-certificates=on tls-cert='${squidCA}'.crt tls-key='${squidCA}'.key \n\
\n\
acl IPv4addr ssl::server_name_regex ^[0-9]+(\\.[0-9]+){0,3}$ \n\
acl IPv6addr ssl::server_name_regex ^[a-fA-F0-9]{0,4}(::?[a-fA-F0-9]{0,4}){0,7}$ \n\
\n\
acl TCPConnection  at_step SslBump1 \n\
acl TLSClientHello at_step SslBump2 \n\
acl TLSServerHello at_step SslBump3 \n\
\n\
ssl_bump peek SSL_ports TCPConnection squidtubeURLs \n\
ssl_bump peek SSL_ports TCPConnection IPv4addr \n\
ssl_bump peek SSL_ports TCPConnection IPv6addr \n\
\n\
ssl_bump bump  SSL_ports TLSClientHello squidtubeURLs \n\
ssl_bump stare SSL_ports TLSClientHello IPv4addr \n\
ssl_bump stare SSL_ports TLSClientHello IPv6addr \n\
\n\
ssl_bump bump      SSL_ports TLSServerHello squidtubeURLs \n\
ssl_bump terminate SSL_ports TLSServerHello IPv4addr \n\
ssl_bump terminate SSL_ports TLSServerHello IPv6addr \n\
\n\
# https://wiki.squid-cache.org/Features/DynamicSslCert \n\
sslcrtd_program '${squidHelpers}'/security_file_certgen -s '${squidData}'/ssl_db -M 4MB \n\
    "' && \
    su -s /bin/bash -l -c "'${squidHelpers}'/security_file_certgen -c -s '${squidData}'/ssl_db -M 4MB" squid

# Configuration of squidtube as a squid ACL external helper
RUN ansible localhost -m lineinfile -a 'path="'${squidConf}'" insertbefore=BOF line="external_acl_type squidtube ipv4 children-max=2 concurrency=4 %URI '${squidtubeBin}'/squidtube --main.loglevel DEBUG"' && \
    ansible localhost -m lineinfile -a 'path="'${squidConf}'" insertbefore=BOF line="strip_query_terms off"'

# Configuration of some ACL examples
RUN ansible localhost -m file -a 'path=/etc/squid/files state=directory group="'${squidGroup}'" mode="0750"' && \
    echo -e '# Rebecca Black'\''s channel \nrebecca' >> /etc/squid/files/YouTube-BlockedChannels.txt && \
    echo -e '# KondZilla'\''s channel \nUCffDXn7ycAzwL2LDlbyWOTw' >> /etc/squid/files/YouTube-BlockedChannels.txt && \
    ansible localhost -m blockinfile -a 'path="'${squidConf}'" marker="# {mark} squidtube ACL configuration block" insertafter="(?i)^\s*#.*\s*INSERT\s+YOUR\s+(.*\s)?RULE\S*\s+HERE(\s.*)?$" block="\
# Block videos whose category is "Gaming" (ID=20) \n\
acl GamingVideo external squidtube snippet.categoryId = 20 \n\
http_access deny squidtubeURLs GamingVideo \n\
\n\
# Block videos whose duration is at least 10 minutes \n\
# https://developers.google.com/youtube/v3/docs/videos#contentDetails.duration \n\
# https://en.wikipedia.org/wiki/ISO_8601#Durations \n\
acl BigVideo external squidtube contentDetails.duration ^P(\\d+Y|\\d+M|\\d+D|T\\d+H|T\\d\\d+M) \n\
http_access deny squidtubeURLs BigVideo \n\
\n\
# Block videos tagged with "NSFW" \n\
acl NSFWTitle external squidtube snippet.title -i \\bnsfw\\b \n\
acl NSFWTag   external squidtube snippet.tags.[] -i \\bnsfw\\b \n\
http_access deny squidtubeURLs NSFWTitle \n\
http_access deny squidtubeURLs NSFWTag \n\
\n\
# Block videos published by a blocked YouTube channel \n\
acl BlockedChannelsID   external squidtube snippet.channelId    "/etc/squid/files/YouTube-BlockedChannels.txt" \n\
acl BlockedChannelsName external squidtube snippet.channelTitle "/etc/squid/files/YouTube-BlockedChannels.txt" \n\
http_access deny squidtubeURLs BlockedChannelsID \n\
http_access deny squidtubeURLs BlockedChannelsName \n\
\n\
# Block live videos \n\
acl LiveVideo external squidtube snippet.liveBroadcastContent live upcoming \n\
http_access deny squidtubeURLs LiveVideo \n\
\n\
# Block age-restricted videos \n\
acl AgeRestrictedVideo external squidtube contentDetails.contentRating.ytRating -i AgeRestrict \n\
http_access deny squidtubeURLs AgeRestrictedVideo \n\
    "'
# Demonstration purposes - Enable youtube squidtube helper only and change the 'part' parameter to include contentDetails
RUN ansible localhost -m ini_file -a 'path="'${squidtubeConf}'/squidtube.conf" section=main option=helpers value="\"youtube\""' && \
    ansible localhost -m ini_file -a 'path="'${squidtubeConf}'/squidtube.conf" section=youtube option=part value="\"snippet,contentDetails\""'

# Removal of packages that is only required to build squidtube
RUN yum -y history undo last

# Definition of a reminder script
RUN touch /etc/profile.d/reminder.sh && \
    chmod 755 /etc/profile.d/reminder.sh && \
    echo -e '#!/bin/bash\n\ncat <<'\''NOTE'\''\n\
**** NOTE ****\n\
\n\
Three additional steps are required in order to have squidtube working: \n\
\n\
1. Get a YouTube API key using instructions located at https://developers.google.com/youtube/registering_an_application#Create_API_Keys \n\
2. In "'${squidtubeConf}'/squidtube.conf", set the parameter "v3ApiKey" in section "youtube" to that API key and reload squid (squid -k reconfigure) \n\
3. Configure your browser to use container'\''s squid instance as HTTP proxy and trust the certificate authority below.\n\
\n\
Squid MITM CA certificate:\n\
\n\
NOTE\n\ncat "'${squidCA}'.crt"\n\necho' > /etc/profile.d/reminder.sh

EXPOSE 3128/tcp

CMD /bin/bash -c '/bin/su -s /bin/bash -l -c "/usr/bin/nohup /usr/sbin/squid -R -F" squid ; exec /bin/bash'
