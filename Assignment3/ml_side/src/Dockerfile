FROM php:8.0-apache
WORKDIR /var/www/html

COPY server.php server.php
RUN mkdir /var/www/data
COPY data.txt /var/www/data/
RUN touch /var/www/data/writeData.txt
RUN chmod 777 /var/www/data/data.txt
RUN chmod 777 /var/www/data/writeData.txt

EXPOSE 80
