FROM nginx:alpine

# Copiar arquivos do frontend
COPY index.html /usr/share/nginx/html/
COPY app.js /usr/share/nginx/html/

# Configuração simples do nginx
RUN echo 'server { \
    listen 80; \
    location / { \
        root /usr/share/nginx/html; \
        index index.html; \
        try_files $uri $uri/ /index.html; \
    } \
}' > /etc/nginx/conf.d/default.conf

EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]
