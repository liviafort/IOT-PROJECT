FROM node:20-alpine

WORKDIR /app

# Copiar package files
COPY package*.json ./
COPY tsconfig.json ./

# Instalar dependências
RUN npm install

# Copiar código fonte
COPY src ./src

# Build TypeScript
RUN npm run build

# Expor porta
EXPOSE 3000

# Start
CMD ["npm", "start"]
