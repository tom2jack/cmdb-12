INSERT INTO packages (package, os_id, varient_id) SELECT 'ntp', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos';
INSERT INTO packages (package, os_id, varient_id) SELECT 'openldap-clients', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos';
INSERT INTO packages (package, os_id, varient_id) SELECT 'openldap-servers', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos';
INSERT INTO packages (package, os_id, varient_id) SELECT 'openldap-servers-sql', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos';
INSERT INTO packages (package, os_id, varient_id) SELECT 'postfix', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos';
INSERT INTO packages (package, os_id, varient_id) SELECT 'nfs-utils', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos';
INSERT INTO packages (package, os_id, varient_id) SELECT 'mysql', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'mysql';
INSERT INTO packages (package, os_id, varient_id) SELECT 'mysql-server', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'mysql';
INSERT INTO packages (package, os_id, varient_id) SELECT 'mysql', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'lamp';
INSERT INTO packages (package, os_id, varient_id) SELECT 'mysql-server', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'lamp';
INSERT INTO packages (package, os_id, varient_id) SELECT 'mozldap', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'ldap';
INSERT INTO packages (package, os_id, varient_id) SELECT 'mozldap-devel', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'ldap';
INSERT INTO packages (package, os_id, varient_id) SELECT 'mozldap-tools', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'ldap';
INSERT INTO packages (package, os_id, varient_id) SELECT 'python-ldap', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'ldap';
INSERT INTO packages (package, os_id, varient_id) SELECT 'php-ldap', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'ldap';
INSERT INTO packages (package, os_id, varient_id) SELECT 'httpd', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'web';
INSERT INTO packages (package, os_id, varient_id) SELECT 'httpd', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'lamp';
INSERT INTO packages (package, os_id, varient_id) SELECT 'php', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'web';
INSERT INTO packages (package, os_id, varient_id) SELECT 'php-mysql', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'web';
INSERT INTO packages (package, os_id, varient_id) SELECT 'mysql', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'web';
INSERT INTO packages (package, os_id, varient_id) SELECT 'php', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'lamp';
INSERT INTO packages (package, os_id, varient_id) SELECT 'php-mysql', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'lamp';
INSERT INTO packages (package, os_id, varient_id) SELECT 'php-bcmath', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'lamp';
INSERT INTO packages (package, os_id, varient_id) SELECT 'php-cli', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'lamp';
INSERT INTO packages (package, os_id, varient_id) SELECT 'php-common', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'lamp';
INSERT INTO packages (package, os_id, varient_id) SELECT 'php-dba', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'lamp';
INSERT INTO packages (package, os_id, varient_id) SELECT 'php-gd', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'lamp';
INSERT INTO packages (package, os_id, varient_id) SELECT 'php-imap', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'lamp';
INSERT INTO packages (package, os_id, varient_id) SELECT 'php-ldap', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'lamp';
INSERT INTO packages (package, os_id, varient_id) SELECT 'php-mbstring', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'lamp';
INSERT INTO packages (package, os_id, varient_id) SELECT 'php-odbc', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'lamp';
INSERT INTO packages (package, os_id, varient_id) SELECT 'php-pdo', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'lamp';
INSERT INTO packages (package, os_id, varient_id) SELECT 'php-pgsql', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'lamp';
INSERT INTO packages (package, os_id, varient_id) SELECT 'php-snmp', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'lamp';
INSERT INTO packages (package, os_id, varient_id) SELECT 'php-soap', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'lamp';
INSERT INTO packages (package, os_id, varient_id) SELECT 'php-xml', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'lamp';
INSERT INTO packages (package, os_id, varient_id) SELECT 'php-xmlrpc', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'lamp';
INSERT INTO packages (package, os_id, varient_id) SELECT 'dovecot', os_id, varient_id FROM build_os bo, varient v WHERE bo.alias = 'centos' AND v.valias = 'email';