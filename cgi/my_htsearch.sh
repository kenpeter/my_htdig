#!/bin/sh

#/var/www/html/my_htdig/cgi/htsearch -v -d -c /var/www/html/my_htdig/htdig/conf/htdig.conf
gdb --args /var/www/html/my_htdig/cgi/htsearch -v -d -c /var/www/html/my_htdig/htdig/conf/htdig.conf 
