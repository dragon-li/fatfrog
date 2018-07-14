
#Optional Features:
#  --disable-option-checking  ignore unrecognized --enable/--with options
#  --disable-FEATURE       do not include FEATURE (same as --enable-FEATURE=no)
#  --enable-FEATURE[=ARG]  include FEATURE [ARG=yes]
#  --enable-maintainer-mode
#                          enable make rules and dependencies not useful (and
#                          sometimes confusing) to the casual installer
#  --enable-silent-rules   less verbose build output (undo: "make V=1")
#  --disable-silent-rules  verbose build output (undo: "make V=0")
#  --enable-debug          Enable debug build options
#  --disable-debug         Disable debug build options
#  --enable-optimize       Enable compiler optimizations
#  --disable-optimize      Disable compiler optimizations
#  --enable-warnings       Enable strict compiler warnings
#  --disable-warnings      Disable strict compiler warnings
#  --enable-werror         Enable compiler warnings as errors
#  --disable-werror        Disable compiler warnings as errors
#  --enable-curldebug      Enable curl debug memory tracking
#  --disable-curldebug     Disable curl debug memory tracking
#  --enable-symbol-hiding  Enable hiding of library internal symbols
#  --disable-symbol-hiding Disable hiding of library internal symbols
#  --enable-hidden-symbols To be deprecated, use --enable-symbol-hiding
#  --disable-hidden-symbols
#                          To be deprecated, use --disable-symbol-hiding
#  --enable-ares[=PATH]    Enable c-ares for DNS lookups
#  --disable-ares          Disable c-ares for DNS lookups
#  --disable-rt            disable dependency on -lrt
#  --enable-dependency-tracking
#                          do not reject slow dependency extractors
#  --disable-dependency-tracking
#                          speeds up one-time build
#  --disable-largefile     omit support for large files
#  --enable-shared[=PKGS]  build shared libraries [default=yes]
#  --enable-static[=PKGS]  build static libraries [default=yes]
#  --enable-fast-install[=PKGS]
#                          optimize for fast installation [default=yes]
#  --disable-libtool-lock  avoid locking (might break parallel builds)
#  --enable-http           Enable HTTP support
#  --disable-http          Disable HTTP support
#  --enable-ftp            Enable FTP support
#  --disable-ftp           Disable FTP support
#  --enable-file           Enable FILE support
#  --disable-file          Disable FILE support
#  --enable-ldap           Enable LDAP support
#  --disable-ldap          Disable LDAP support
#  --enable-ldaps          Enable LDAPS support
#  --disable-ldaps         Disable LDAPS support
#  --enable-rtsp           Enable RTSP support
#  --disable-rtsp          Disable RTSP support
#  --enable-proxy          Enable proxy support
#  --disable-proxy         Disable proxy support
#  --enable-dict           Enable DICT support
#  --disable-dict          Disable DICT support
#  --enable-telnet         Enable TELNET support
#  --disable-telnet        Disable TELNET support
#  --enable-tftp           Enable TFTP support
#  --disable-tftp          Disable TFTP support
#  --enable-pop3           Enable POP3 support
#  --disable-pop3          Disable POP3 support
#  --enable-imap           Enable IMAP support
#  --disable-imap          Disable IMAP support
#  --enable-smb            Enable SMB/CIFS support
#  --disable-smb           Disable SMB/CIFS support
#  --enable-smtp           Enable SMTP support
#  --disable-smtp          Disable SMTP support
#  --enable-gopher         Enable Gopher support
#  --disable-gopher        Disable Gopher support
#  --enable-manual         Enable built-in manual
#  --disable-manual        Disable built-in manual
#  --enable-libcurl-option Enable --libcurl C code generation support
#  --disable-libcurl-option
#                          Disable --libcurl C code generation support
#  --enable-libgcc         use libgcc when linking
#  --enable-ipv6           Enable IPv6 (with IPv4) support
#  --disable-ipv6          Disable IPv6 support
#  --enable-versioned-symbols
#                          Enable versioned symbols in shared library
#  --disable-versioned-symbols
#                          Disable versioned symbols in shared library
#  --enable-threaded-resolver
#                          Enable threaded resolver
#  --disable-threaded-resolver
#                          Disable threaded resolver
#  --enable-verbose        Enable verbose strings
#  --disable-verbose       Disable verbose strings
#  --enable-sspi           Enable SSPI
#  --disable-sspi          Disable SSPI
#  --enable-crypto-auth    Enable cryptographic authentication
#  --disable-crypto-auth   Disable cryptographic authentication
#  --enable-ntlm-wb[=FILE] Enable NTLM delegation to winbind's ntlm_auth
#                          helper, where FILE is ntlm_auth's absolute filename
#                          (default: /usr/bin/ntlm_auth)
#  --disable-ntlm-wb       Disable NTLM delegation to winbind's ntlm_auth
#                          helper
#  --enable-tls-srp        Enable TLS-SRP authentication
#  --disable-tls-srp       Disable TLS-SRP authentication
#  --enable-unix-sockets   Enable Unix domain sockets
#  --disable-unix-sockets  Disable Unix domain sockets
#  --enable-cookies        Enable cookies support
#  --disable-cookies       Disable cookies support
#  --enable-soname-bump    Enable enforced SONAME bump
#  --disable-soname-bump   Disable enforced SONAME bump
#
#Optional Packages:
#  --with-PACKAGE[=ARG]    use PACKAGE [ARG=yes]
#  --without-PACKAGE       do not use PACKAGE (same as --with-PACKAGE=no)
#  --with-pic[=PKGS]       try to use only PIC/non-PIC objects [default=use
#                          both]
#  --with-aix-soname=aix|svr4|both
#                          shared library versioning (aka "SONAME") variant to
#                          provide on AIX, [default=aix].
#  --with-gnu-ld           assume the C compiler uses GNU ld [default=no]
#  --with-sysroot[=DIR]    Search for dependent libraries within DIR (or the
#                          compiler's sysroot if not specified).
#  --with-zlib=PATH        search for zlib in PATH
#  --without-zlib          disable use of zlib
#  --with-ldap-lib=libname Specify name of ldap lib file
#  --with-lber-lib=libname Specify name of lber lib file
#  --with-gssapi-includes=DIR
#                          Specify location of GSS-API headers
#  --with-gssapi-libs=DIR  Specify location of GSS-API libs
#  --with-gssapi=DIR       Where to look for GSS-API
#  --with-winssl           enable Windows native SSL/TLS
#  --without-winssl        disable Windows native SSL/TLS
#  --with-darwinssl        enable iOS/Mac OS X native SSL/TLS
#  --without-darwinssl     disable iOS/Mac OS X native SSL/TLS
#  --with-ssl=PATH         Where to look for OpenSSL, PATH points to the SSL
#                          installation (default: /usr/local/ssl); when
#                          possible, set the PKG_CONFIG_PATH environment
#                          variable instead of using this option
#  --without-ssl           disable OpenSSL
#  --with-egd-socket=FILE  Entropy Gathering Daemon socket pathname
#  --with-random=FILE      read randomness from FILE (default=/dev/urandom)
#  --with-gnutls=PATH      where to look for GnuTLS, PATH points to the
#                          installation root
#  --without-gnutls        disable GnuTLS detection
#  --with-polarssl=PATH    where to look for PolarSSL, PATH points to the
#                          installation root
#  --without-polarssl      disable PolarSSL detection
#  --with-mbedtls=PATH     where to look for mbedTLS, PATH points to the
#                          installation root
#  --without-mbedtls       disable mbedTLS detection
#  --with-cyassl=PATH      where to look for CyaSSL, PATH points to the
#                          installation root (default: system lib default)
#  --without-cyassl        disable CyaSSL detection
#  --with-nss=PATH         where to look for NSS, PATH points to the
#                          installation root
#  --without-nss           disable NSS detection
#  --with-axtls=PATH       Where to look for axTLS, PATH points to the axTLS
#                          installation prefix (default: /usr/local). Ignored
#                          if another SSL engine is selected.
#  --without-axtls         disable axTLS
#  --with-ca-bundle=FILE   Path to a file containing CA certificates (example:
#                          /etc/ca-bundle.crt)
#  --without-ca-bundle     Don't use a default CA bundle
#  --with-ca-path=DIRECTORY
#                          Path to a directory containing CA certificates
#                          stored individually, with their filenames in a hash
#                          format. This option can be used with OpenSSL, GnuTLS
#                          and PolarSSL backends. Refer to OpenSSL c_rehash for
#                          details. (example: /etc/certificates)
#  --without-ca-path       Don't use a default CA path
#  --with-ca-fallback      Use the built in CA store of the SSL library
#  --without-ca-fallback   Don't use the built in CA store of the SSL library
#  --without-libpsl        disable support for libpsl cookie checking
#  --with-libmetalink=PATH where to look for libmetalink, PATH points to the
#                          installation root
#  --without-libmetalink   disable libmetalink detection
#  --with-libssh2=PATH     Where to look for libssh2, PATH points to the
#                          LIBSSH2 installation; when possible, set the
#                          PKG_CONFIG_PATH environment variable instead of
#                          using this option
#  --without-libssh2       disable LIBSSH2
#  --with-librtmp=PATH     Where to look for librtmp, PATH points to the
#                          LIBRTMP installation; when possible, set the
#                          PKG_CONFIG_PATH environment variable instead of
#                          using this option
#  --without-librtmp       disable LIBRTMP
#  --with-winidn=PATH      enable Windows native IDN
#  --without-winidn        disable Windows native IDN
#  --with-libidn=PATH      Enable libidn usage
#  --without-libidn        Disable libidn usage
#  --with-nghttp2=PATH     Enable nghttp2 usage
#  --without-nghttp2       Disable nghttp2 usage
#  --with-zsh-functions-dir=PATH
#                          Install zsh completions to PATH
#  --without-zsh-functions-dir
#                          Do not install zsh completions
#
./configure               \
  --enable-http           \
  --disable-file          \
  --disable-file          \
  --disable-rtsp          \
  --disable-ldap          \
  --disable-ldaps         \
  --disable-proxy         \
  --disable-dict          \
  --disable-telnet        \
  --disable-tftp          \
  --disable-pop3          \
  --disable-imap          \
  --disable-smb           \
  --disable-smtp          \
  --disable-gopher        \
  --disable-manual        \
  --disable-ares          \
  --disable-threaded-resolver \
  --disable-ipv6          \
  --enable-libcurl-option \
  --enable-verbose        \
  --disable-sspi          \
  --enable-crypto-auth   \
  --disable-ntlm-wb       \
  --disable-tls-srp       \
  --disable-unix-sockets  \
  --enable-cookies        \
  --disable-soname-bump   \

