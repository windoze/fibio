DROP DATABASE IF EXISTS test;
CREATE DATABASE test;
GRANT all ON test.* TO testuser@'localhost' IDENTIFIED BY 'testpass';
