FROM nginx:alpine

# Copiar arquivos do frontend
COPY index.html /usr/share/nginx/html/
COPY app.js /usr/share/nginx/html/

# Configuração do nginx com prefixo /iot/frontend
RUN echo 'server { \
    listen 80; \
    location /iot/frontend { \
        alias /usr/share/nginx/html; \
        index index.html; \
        try_files $uri $uri/ /iot/frontend/index.html; \
    } \
    location = /iot/frontend/ { \
        alias /usr/share/nginx/html/; \
        index index.html; \
    } \
}' > /etc/nginx/conf.d/default.conf

EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]
