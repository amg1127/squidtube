# Squidtube

Squidtube is an external Squid ACL class helper that provides control over access to videos. With Squidtube, a Squid administrator is able to define policies that allow or deny accesses to content published on supported media portals and preserve some network bandwidth as a consequence. Squidtube currently supports YouTube only.

Squidtube is licensed under GNU General Public License terms, version 3 or later.

## 1. Introduction

Content filtering with Squid works by comparing users' request properties, such as URL domain, URL path, timestamp, etc., against [ACLs](http://www.squid-cache.org/Doc/config/acl/) written by the system administrator. Some media portals, however, offer additional data related to requested contents through application programming interfaces (APIs). For example, YouTube Data API offers a method named [`Videos:list`](https://developers.google.com/youtube/v3/docs/videos/list), which provides video attributes such as title, description, uploader's channel ID, tags and category. Those additional attributes can also be evaluated within Squid by configuring Squidtube as an external class helper, thus permitting configuration of access policies like these:

* Denying access to videos whose category is *Gaming*;
* Allowing access to videos published by a specific set of uploader ID's and denying everything else;
* Denying access to videos with a particular tag;
* Allowing access to videos with at least 1 million views only.

The following steps describe the communication flow between Squidtube and Squid:

* The user requests content from a media portal through a web browser;
* Squid evaluates user's request and extracts its URL;
* Squid passes the requested URL and defined ACL entries to Squidtube through [helper's interface](https://wiki.squid-cache.org/Features/AddonHelpers#Access_Control_.28ACL.29);
* A Squidtube's helper parses the URL, identifies the video the user is actually requesting and, if needed, retrieves metadata from website's API;
* Squidtube evaluates video's metadata and ACL entries and returns evaluation responses to Squid;
* Squid either allows or denies the request, according to answers received from Squidtube and policies written in configuration.

A Squidtube's helper is a component file written in Javascript language that recognizes objects (images, videos, etc) from a media portal and retrieves metadata using website's API and [XMLHttpRequest](https://en.wikipedia.org/wiki/XMLHttpRequest) objects. During the initialization phase, Squidtube loads helpers either from the architecture-independent data directory or from the configuration directory (the latter overrides the former). Then, after Squidtube receives URL's and ACL entries from Squid, Squidtube firstly selects a helper according to URL patterns the helper declares as supported and request it to return a recognized object class and identifier. Afterwards, Squidtube requests the helper to return object's metadata from the portal's API (unless fresh cached data can be retrieved from a database), evaluates such information against the ACL entries and returns a matching response to Squid. The communication between Squidtube and its helpers flows through Javascript functions defined according to Squidtube's internal protocol.

## 2. Installation from sources

Squidtube depends on [QT5 library](https://www.qt.io/) version 5.5.0 or above. In order to build Squidtube successfully, a C++ compiler and a working QT5 installation are needed. From the QT5 library, Squidtube requires specifically the QtBase module (Qt Core, Qt Network and Qt SQL submodules) and the QtDeclarative module (Qt QML submodule).

Assuming that QT5 tools are available in ${PATH}, issue under Squidtube's extracted source tree the following command sequence:

```
# qmake squidtube.pro
# make
# make install
```

Under a GNU/Linux operating system, Squidtube installs the binary executable under `/usr/local/bin/`, a configuration file under `/usr/local/etc/squidtube` and architecture-independent files (helpers and Javascript libraries) under `/usr/local/share/squidtube`. Under Microsoft Windows, Squidtube installs the binary executable and a configuration file under `C:\Program Files\amg1127\squidtube` and architecture-independent files under `C:\Program Data\amg1127\squidtube`.

The installation paths can be changed by specifying additional command line arguments to `qmake`. For example, in order to use `/usr` as installation prefix instead of `/usr/local`, replace the `qmake` command line shown above by:

```
# qmake squidtube.pro install_bin_dir=/usr/bin install_etc_dir=/etc/squidtube install_share_dir=/usr/share/squidtube
```

Squidtube sources also recognize the `make test` command, which can be optionally issued for testing the program itself under the running operating system. The included test suite requires a working command-line [PHP](http://www.php.net/) installation, version 5.4.0 or above.

## 3. Configuration

### 3.1. Squidtube's configuration file

Unless specified otherwise, Squidtube reads configuration parameters from the file `/usr/local/etc/squidtube/squidtube.conf` (`${install_etc_dir}/squidtube.conf`, actually). As stated above, the default configuration file can be changed at build time by supplying a value to `install_etc_dir` variable through a command line argument to `qmake`. A Squid administrator can also reference an alternative configuration file for Squidtube, by using the `--config` command line parameter:

```
/usr/local/bin/squidtube --config /path/to/alternative/squidtube.conf
```

Squidtube's configuration file uses the [INI file format](https://en.wikipedia.org/wiki/INI_file). The `[main]` section defines global settings for Squidtube while the `[db]` section stores database connection parameters. Additionally, for each helper loaded by Squidtube, if there is a configuration section whose name matches helper's name, then Squidtube reads all keys and values from that section and presents them to the helper as Javascript variables. For example, a `[youtube]` section would store parameters used by the `youtube` helper.

Command line arguments can override parameters parsed from the configuration file by using the syntax `--<section>.<key> <value>`. For example, the logging level can be temporary raised for troubleshooting purposes by using the command line argument `--main.loglevel DEBUG`.

Squidtube recognizes the following keys from the `[main]` section:

* `loglevel` - The logging level used by Squidtube. The parameter accepts one of these strings: `DEBUG`, `INFO`, `WARNING` and `ERROR`. Squidtube uses its standard error stream for debugging purposes.
* `helpers` - A comma-separated list of helpers that Squidtube will load from either the architecture-independent data directory or from the configuration directory.
* `positiveTTL` - Squidtube reuses cached object metadata if it is no older than the number of seconds specified by the parameter. That helps reduce calls to media portal's APIs.
* `negativeTTL` - Metadata retrieval failures from a helper are recorded by Squidtube for the number of seconds specified by the parameter.
* `protocol` - The protocol compatibility mode to use while retrieving and answering requests from Squid. Set it to `3.4` if using Squid >= 3.4 or `3.0` if using Squid >= 3.0 and < 3.4.

In order to reduce the number API requests, Squidtube is designed to temporarily store object metadata received by helpers into a database. In a Squidtube deployment plan, prefer to share an unique database if more than one Squidtube process is to be run, as it will help avoid duplicate API requests to media portals. Note in this case that if multiple Squidtube processes are to be run on different systems, then their clocks have to be synchronized with a common time source.

Database connection settings are loaded from the `[db]` section. The following keys are recognized:

* `driver` - SQL driver to use for database access. The parameter value has to be a driver name recognized by the QT5 library's SQL framework. Supported names are listed at [QSqlDatabase::addDatabase method documentation](http://doc.qt.io/qt-5/qsqldatabase.html#addDatabase-1).
* `hostname` - Hostname of the database server Squidtube will connect to.
* `port` - Port number that Squidtube will use for database connection.
* `username` and `password` - Authentication credentials that the Squidtube will present to the database server while the connection is being negociated.
* `options` - Database-specific connection options used by the SQL driver. See [QSqlDatabase::setConnectOptions method documentation](http://doc.qt.io/qt-5/qsqldatabase.html#setConnectOptions) for details about supported options and syntax.
* `name` - Name of the database that will be used by Squidtube. For a SQLITE database, for example, this parameter sets the database file path.
* `tableprefix` - Table prefix for tables that will be used by Squidtube. The actual table names are formed by the configured prefix plus an underscore (`_`) plus the helper name. Squidtube uses a table for each active helper.
* `startup` - A semicolon-separated list of SQL queries that Squidtube will submit to the database after a successful connection and before issuing data manipulation queries.

Documentation for configuration keys recognized by helpers are found in comment blocks within the helpers themselves. Credentials for API access are usually required, so it's advised to check helpers' source codes located under the architecture-independent data directory (which is either `/usr/local/share/squidtube/helpers` or `C:\Program Data\amg1127\squidtube\helpers` by default).

### 3.2. HTTPS traffic decryption by Squid

The majority of media portals uses secure HTTP. Therefore, Squidtube will be able to filter media requests from users only if Squid is configured to decrypt HTTPS requests. Before using Squidtube, be aware of all technical and legal implications derived from a *Squid-in-the-middle HTTPS decryption*.

HTTPS decryption feature was first made available in Squid 3.1 and has changed throughout the time. For Squid versions 3.1 or 3.2, request decryption is achieved through the [SslBump](https://wiki.squid-cache.org/Features/SslBump) feature. For example:

```
http_port 3128 ssl-bump generate-host-certificates=on cert=/usr/local/etc/squid/SslBump-cert+key.pem

acl SSL_ports port 443

acl squidtubeURLs dstdom_regex \.youtu\.be$
acl squidtubeURLs dstdom_regex \.youtube\.com$
acl squidtubeURLs dstdom_regex \.youtube\.com\.[a-z][a-z]$

ssl_bump allow SSL_ports squidtubeURLs

always_direct allow all
```

For Squid versions 3.3 and 3.4, the [BumpSslServerFirst](https://wiki.squid-cache.org/Features/BumpSslServerFirst) feature is the preferred way for HTTPS decryption setup. For example:

```
http_port 3128 ssl-bump generate-host-certificates=on cert=/usr/local/etc/squid/SslBumpServerFirst-cert+key.pem

acl SSL_ports port 443

acl squidtubeURLs dstdom_regex \.youtu\.be$
acl squidtubeURLs dstdom_regex \.youtube\.com$
acl squidtubeURLs dstdom_regex \.youtube\.com\.[a-z][a-z]$

ssl_bump server-first SSL_ports squidtubeURLs

# https://wiki.squid-cache.org/Features/DynamicSslCert
sslcrtd_program /usr/local/squid/libexec/ssl_crtd -s /usr/local/squid/var/lib/ssl_db -M 4MB
```

For Squid version 3.5, the preferred HTTPS decryption feature is [SslPeekAndSplice](https://wiki.squid-cache.org/Features/SslPeekAndSplice). For example:

```
http_port 3128 ssl-bump generate-host-certificates=on cert=/etc/squid/SslPeekAndSplice-cert+key.pem

acl SSL_ports port 443

acl squidtubeURLs ssl::server_name_regex \.youtu\.be$
acl squidtubeURLs ssl::server_name_regex \.youtube\.com$
acl squidtubeURLs ssl::server_name_regex \.youtube\.com\.[a-z][a-z]$

acl IPv4addr ssl::server_name_regex ^[0-9]+(\.[0-9]+){0,3}$
acl IPv6addr ssl::server_name_regex ^[a-fA-F0-9]{0,4}(::?[a-fA-F0-9]{0,4}){0,7}$

acl TCPConnection  at_step SslBump1
acl TLSClientHello at_step SslBump2
acl TLSServerHello at_step SslBump3

ssl_bump peek SSL_ports TCPConnection squidtubeURLs
ssl_bump peek SSL_ports TCPConnection IPv4addr
ssl_bump peek SSL_ports TCPConnection IPv6addr

ssl_bump bump  SSL_ports TLSClientHello squidtubeURLs
ssl_bump stare SSL_ports TLSClientHello IPv4addr
ssl_bump stare SSL_ports TLSClientHello IPv6addr

ssl_bump bump      SSL_ports TLSServerHello squidtubeURLs
ssl_bump terminate SSL_ports TLSServerHello IPv4addr
ssl_bump terminate SSL_ports TLSServerHello IPv6addr

# https://wiki.squid-cache.org/Features/DynamicSslCert
sslcrtd_program /usr/local/squid/libexec/ssl_crtd -s /usr/local/squid/var/lib/ssl_db -M 4MB
```

Check reference documentation related to HTTPS decryption before applying any of the above configuration snippets into the Squid installation. The following web pages are suggested:

* [Features/SslBump](https://wiki.squid-cache.org/Features/SslBump)
* [Features/BumpSslServerFirst](https://wiki.squid-cache.org/Features/BumpSslServerFirst)
* [Features/SslPeekAndSplice](https://wiki.squid-cache.org/Features/SslPeekAndSplice)
* [Features/DynamicSslCert](https://wiki.squid-cache.org/Features/DynamicSslCert)
* [Doc/config/acl](http://www.squid-cache.org/Doc/config/acl/)
* [Doc/config/ssl_bump](http://www.squid-cache.org/Doc/config/ssl_bump/)

### 3.3. Integration as an external list

Squid communicates with Squidtube through [external ACLs](https://wiki.squid-cache.org/Features/AddonHelpers#Access_Control_.28ACL.29). In order to configure Squidtube as an external ACL handler for content filtering, the [external_acl_type](http://www.squid-cache.org/Doc/config/external_acl_type/) configuration directive is used. For example:

```
# For Squid >= 3.4
external_acl_type squidtube children-max=2 concurrency=4 %URI /usr/local/bin/squidtube

# For Squid >= 3.0 and < 3.4
#external_acl_type squidtube children-max=2 concurrency=4 %URI /usr/local/bin/squidtube --main.protocol 3.0
```

Note that `%URI` must be the only explicitly defined macro in the external ACL type definition. Also note that (a) Squidtube does not support Squid protocol version 2.5; and (b) Squid < 3.4 requires setting `main.protocol` configuration parameter to `3.0`.

Squidtube is capable of processing more than one query at a time by using one thread of execution per request channel. Therefore, setting a concurrency level per process (`concurrency=` argument) is recommended, as it helps achieve request handling concurrency using less resource allocation than multiple Squidtube processes.

### 3.4. Creating ACL entries

The syntax for writing Squidtube ACL entries into Squid configuration file is defined below:

```
acl <ACLNAME> external squidtube <PROPERTY> [!] [<OPERATOR>] [<FLAG> [<FLAG>]] [--] <ARGUMENT> [<ARGUMENT> [<ARGUMENT> [...] ] ]
acl <ACLNAME> external squidtube <PROPERTY> [!] [<OPERATOR>] [<FLAG> [<FLAG>]] [--] "<FILE>"
```

As commonly used in other ACL types, the former specifies arguments in-place while the latter allows arguments to be loaded from an input file.

The definition of the fields are:

* `<ACLNAME>` - A name that will reference the ACL entry within zero or more [http_access](http://www.squid-cache.org/Doc/config/http_access/) directives.
* `<PROPERTY>` - Specifies which attribute from video's metadata will be evaluated. Content metadata that a Squidtube's helper collects from the media portal's API is transferred to Squidtube using the JSON format and `<PROPERTY>` is an expression which describes what property from that JSON object will be selected for evaluation.
* `!` - Logical negation operator. If specified, ACL entry evaluation result is inverted, that is, a `MATCH` result becomes a `NOMATCH` and *vice-versa*.
* `<OPERATOR>` - Optional comparison operator:
    * *(not specified)* - The ACL entry evaluates to `MATCH` if property value is **string** and matches at least one argument pattern. The matching test is either *perl-compatible regular expression*, *wildcard expression* or *fixed string comparison*, according to the *flags* that are specified. For **numeric** and **boolean** property values, an operator abscence behaves as if the `==` operator was specified.
    * `=`, `==` or `-eq` - The ACL entry evaluates to `MATCH` if and only if property value is **equals to** at least one of the arguments.
    * `!=`, `<>` or `-ne` - The ACL entry evaluates to `MATCH` if and only if property value is **different from** at least one of the arguments.
    * `<` or `-lt` - The ACL entry evaluates to `MATCH` if and only if property value is **numeric** or **string** and it is **less than** at least one of the arguments.
    * `<=` or `-le` - The ACL entry evaluates to `MATCH` if and only if property value is **numeric** or **string** and it is **less than or equals to** at least one of the arguments.
    * `>` or `-gt` - The ACL entry evaluates to `MATCH` if and only if property value is **numeric** or **string** and it is **greater than** at least one of the arguments.
    * `>=` or `-ge` - The ACL entry evaluates to `MATCH` if and only if property value is **numeric** or **string** and it is **greater than or equals to** at least one of the arguments.
* `<FLAG>` - Optional flag that changes how comparisons between **string** property values and arguments are made:
    * *(not specified)* - Arguments will be treated as *perl-compatible regular expressions* and the ACL entry will make case-sensitive matching test between values and each argument pattern.
    * `-i` or `--ignorecase` - Makes all string comparison tests (PCRE, less than, etc...) case-insensitive.
    * `-w` or `--wildcard` - Arguments will be treated as *wildcards expressions*, that is, `?`, `*` and `[...]` will be interpreted as *exactly one character*, *zero or more characters* and *character set*, respectively. This flag must not be used with neither `-f`, `--fixed` nor a comparison operator.
    * `-f` or `--fixed` - Arguments will be treated as they are and the ACL entry will just compare property values to the arguments. This flag must not be used with neither `-w`, `--wildcard` nor a comparison operator.
* `--` Used to stop flag processing, in the case the first argument has a hyphen character as first character.
* `<ARGUMENT>` - Either a string, a number or a boolean keyword that property values will be compared to, according to specified operator and/or flags. The boolean keywords `no`, `0` and `false` evaluate as the boolean value *false* and `yes`, `1` and `true` evaluate as the boolean value *true*. ACL entry evaluates to `MATCH` if and only if at least one comparision test succeeds.
* `<FILE>` - Arguments to the ACL entry can be read from a file instead of being enumerated directly in Squid's configuration file. This field specifies the file path to read from. When using `<FILE>`, the file should contains one argument per line.

`<PROPERTY>` is an expression composed by one or more identifiers separated by dots (`.`), while an identifier is either an object property name or a Squidtube-specific expression that handles array elements (which is similar to the way the [jq](https://stedolan.github.io/jq/) language handles array elements). For example, let us suppose a helper returns the following JSON metadata:

```
{
    "kind": "youtube#video",
    "etag": "\"XI7nbFXulYBIpL0ayR_gDh3eu1k/t0zJFWfepvnU_QV-CCKm8eMTDvI\"",
    "id": "x1LZVmn3p3o",
    "snippet": {
        "publishedAt": "2005-06-26T05:40:44.000Z",
        "channelId": "UCAkqfD0PnSi5oR5ewfcbalA",
        "title": "tow chinese boys:i want it that way",
        "description": "it is funny.\n\nMy Blog: http://www.rapbull.net",
        "thumbnails": {
            "default": {
                "url": "https://i.ytimg.com/vi/x1LZVmn3p3o/default.jpg",
                "width": 120,
                "height": 90
            },
            "medium": {
                "url": "https://i.ytimg.com/vi/x1LZVmn3p3o/mqdefault.jpg",
                "width": 320,
                "height": 180
            },
            "high": {
                "url": "https://i.ytimg.com/vi/x1LZVmn3p3o/hqdefault.jpg",
                "width": 480,
                "height": 360
            }
        },
        "channelTitle": "Cow",
        "tags": [
            "chineseboys",
            "funny",
            "music",
            "China",
            "Singing"
        ],
        "categoryId": "24",
        "liveBroadcastContent": "none",
        "localized": {
            "title": "tow chinese boys:i want it that way",
            "description": "it is funny.\n\nMy Blog: http://www.rapbull.net"
        }
    },
    "contentDetails": {
        "duration": "PT4M56S",
        "dimension": "2d",
        "definition": "sd",
        "caption": "false",
        "licensedContent": false,
        "projection": "rectangular"
    },
    "statistics": {
        "viewCount": "6787612",
        "likeCount": "33968",
        "dislikeCount": "1962",
        "favoriteCount": "0",
        "commentCount": "8415"
    }
}
```

Attributes that are not array members are selected using property names. For example:
* The expression `snippet.title` selects: `tow chinese boys:i want it that way`.
* The expression `snippet.thumbnails.high.width` selects: `480`.
* The expression `snippet.["thumbnails"].["medium"].height` selects: `180`.
* The expression `contentDetails.licensedContent` selects: `false`.

For arrays members, array elements are firstly selected and then compared individually to the ACL entry arguments. The syntax for elements selection is *zero or more specifications of individual indexes or closed intervals, separated by a comma* and the syntax for intervals is *a pair of indexes separated by a colon*. A non-negative index starts counting from the first element and a negative index starts counting from the last. For example, from the array attribute `snippet.tags` defined above:
* The selection expression `0` selects: `chineseboys`
* The selection expression `1` selects: `funny`
* The selection expression `1,3` selects: `funny`, `China`
* The selection expression `1:3` selects: `funny`, `music`, `China`
* The selection expression `0,2,4` selects: `chineseboys`, `music`, `Singing`
* The selection expression `0,2:4` selects: `chineseboys`, `music`, `China`, `Singing`
* The selection expression `0:2,2:4` selects: `chineseboys`, `funny`, `music`, `China`, `Singing`
* The selection expression `-1` selects: `Singing`
* The selection expression `-2` selects: `China`
* The selection expression `-4,-2` selects: `funny`, `China`
* The selection expression `-4:-2` selects: `funny`, `music`, `China`
* The selection expression `-4:2` selects: `funny`, `music`
* The selection expression `-4:4` selects: `funny`, `music`, `China`, `Singing`
* The selection expression `0,-1` selects: `chineseboys`, `Singing`
* The selection expression `0:-1` selects: `chineseboys`, `funny`, `music`, `China`, `Singing`
* The empty selection expression ` ` behaves as `0:-1`, that is, it selects: `chineseboys`, `funny`, `music`, `China`, `Singing`

Identifiers that select array elements must be enclosed by either square brackets (`[ ]`) or angle brackets (`< >`). The brackets define how Squidtube combines results of individual comparisons:
* `[`*(expression)*`]` - An selection expression enclosed by square brackets combines results using a logical  `OR` operation, that is, Squidtube will evaluate selected array elements and return a `MATCH` result if and only if it finds at least one element whose comparison result is `MATCH`.
* `<`*(expression)*`>` - An selection expression enclosed by angle brackets combines results using a logical  `AND` operation, that is, Squidtube will evaluate selected array elements and return a `NOMATCH` result if and only if it finds at least one element whose comparison result is `NOMATCH`.


#### 3.4.1. Configuration snippet examples - content blocking

```
# This directive configures Squidtube as an external ACL handler
external_acl_type squidtube children-max=2 concurrency=4 %URI /usr/local/bin/squidtube

# These ACL entries match videos from YouTube
acl squidtubeURLs ssl::server_name_regex \.youtu\.be$
acl squidtubeURLs ssl::server_name_regex \.youtube\.com$
acl squidtubeURLs ssl::server_name_regex \.youtube\.com\.[a-z][a-z]$

# Block videos whose category is "Gaming" (ID=20)
acl GamingVideo external squidtube snippet.categoryId = 20
http_access deny squidtubeURLs GamingVideo

# Block videos whose duration is at least 10 minutes
# https://developers.google.com/youtube/v3/docs/videos#contentDetails.duration
# https://en.wikipedia.org/wiki/ISO_8601#Durations
acl BigVideo external squidtube contentDetails.duration ^P(\d+Y|\d+M|\d+D|T\d+H|T\d\d+M)
http_access deny squidtubeURLs BigVideo

# Block videos tagged with "NSFW"
acl NSFWTitle external squidtube snippet.title -i \bnsfw\b
acl NSFWTag   external squidtube snippet.tags.[] -i \bnsfw\b
http_access deny squidtubeURLs NSFWTitle
http_access deny squidtubeURLs NSFWTag

# Block videos published by a blocked YouTube channel
acl BlockedChannelsID   external squidtube snippet.channelId    "/etc/squid/files/YouTube-BlockedChannels.txt"
acl BlockedChannelsName external squidtube snippet.channelTitle "/etc/squid/files/YouTube-BlockedChannels.txt"
http_access deny squidtubeURLs BlockedChannelsID
http_access deny squidtubeURLs BlockedChannelsName

# Block live videos
acl LiveVideo external squidtube snippet.liveBroadcastContent live upcoming
http_access deny squidtubeURLs LiveVideo

# Block age-restricted videos
acl AgeRestrictedVideo external squidtube contentDetails.contentRating.ytRating -i AgeRestrict
http_access deny squidtubeURLs AgeRestrictedVideo
```

#### 3.4.2. Configuration snippet examples - access restriction

ACL entries that restrict access to a configured content subset must be constructed carefully. Otherwise, Squidtube would incorrectly block HTTP requests for page components such as images, javascript files and stylesheet files. For example, although the snippet below intends to restrict access to videos whose category is "Education", it would block YouTube's main page too:

```
# This directive configures Squidtube as an external ACL handler
external_acl_type squidtube children-max=2 concurrency=4 %URI /usr/local/bin/squidtube

# These ACL entries match videos from YouTube
acl squidtubeURLs ssl::server_name_regex \.youtu\.be$
acl squidtubeURLs ssl::server_name_regex \.youtube\.com$
acl squidtubeURLs ssl::server_name_regex \.youtube\.com\.[a-z][a-z]$

# Only allow videos whose category is "Education" (ID=27)
acl EducationalVideo external squidtube snippet.categoryId = 27
http_access deny squidtubeURLs !EducationalVideo
```

Also, connection failures to YouTube API endpoint would block access to videos entirely.
In order to properly restrict access to educational videos, the ACL entry should be rewritten to:

```
# Block videos whose category is not "Education" (ID=27)
acl VideoCategory    external squidtube snippet.categoryId .
acl EducationalVideo external squidtube snippet.categoryId = 27
http_access deny squidtubeURLs VideoCategory !EducationalVideo
```

These are additional examples:

```
# Only allow a specific video subset
acl VideoID       external squidtube id .
acl AllowedVideos external squidtube id -f "/etc/squid/files/YouTube-AllowedVideos.txt"
http_access deny squidtubeURLs VideoID !AllowedVideos

# Only allow videos published by an approved YouTube Channel
acl ChannelID           external squidtube snippet.channelId    .
acl AllowedChannelsID   external squidtube snippet.channelId -f "/etc/squid/files/YouTube-AllowedChannels.txt"
acl ChannelName         external squidtube snippet.channelTitle    .
acl AllowedChannelsName external squidtube snippet.channelTitle -f "/etc/squid/files/YouTube-AllowedChannels.txt"
http_access allow squidtubeURLs AllowedChannelsID
http_access allow squidtubeURLs AllowedChannelsName
http_access deny  squidtubeURLs ChannelID
http_access deny  squidtubeURLs ChannelName

# Only allow videos whose category is either "Education" (ID=27) or "News & Politics" (ID=25)
acl VideoCategory external squidtube snippet.categoryId .
acl AllowedVideos external squidtube snippet.categoryId = 27 25
http_access deny squidtubeURLs VideoCategory !AllowedVideos
```

## 4. Troubleshooting

These are troubleshooting tips that may be used if Squidtube is not working as expected.

### 4.1. Check Squid's log file

Squidtube uses STDERR for debugging purposes. After configured as an external ACL handler, debugging messages are saved into [Squid's administrative logging file](http://www.squid-cache.org/Doc/config/cache_log/), which is usually `/var/log/squid/cache.log` or `/usr/local/squid/var/logs/cache.log`. Therefore, one can monitor Squidtube's behavior from there:

```
# tail -F "${SQUID_LOG_FILE}" | egrep '(FATAL|ERROR|WARNING|INFO|DEBUG)'
```

### 4.2. Preserve query terms from HTTP requests

By default, Squid strips query terms from requested URLs before logging, thus hiding the complete URL from Squid's error pages. For troubleshoot purposes, Squid may be configured to preserve query terms by disabling the parameter [strip_query_terms](http://www.squid-cache.org/Doc/config/strip_query_terms/) in Squid's configuration file.

### 4.3. Increase Squidtube's logging level

Additional troubleshooting messages are emitted by raising the log level via the `main.loglevel` configuration parameter. It can be modified through Squidtube's configuration file:

```
# /usr/local/etc/squidtube/squidtube.conf

[main]
loglevel = DEBUG
```

It can also be modified via command line argument:

```
# /usr/local/etc/squid/squid.conf

external_acl_type squidtube children-max=2 concurrency=4 %URI /usr/local/bin/squidtube --main.loglevel DEBUG
```

### 4.4. Run Squidtube from command line

*(unfinished)*

### 4.5. Recompile Squidtube in debug mode

*(unfinished)*

## 5. Extending Squidtube

*(unfinished)*
