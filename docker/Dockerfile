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

# Configuration of the entire environment via Ansible
COPY ansible-playbook.yml /tmp/
RUN cd /tmp && ansible-playbook -v -c local ansible-playbook.yml

# Removal of Ansible and packages that are only required to build squidtube
RUN yum -y history undo last

EXPOSE 3128/tcp

CMD /bin/bash -c '/bin/su -s /bin/bash -l -c "/usr/bin/nohup /usr/sbin/squid -R -F" squid ; exec /bin/bash'
