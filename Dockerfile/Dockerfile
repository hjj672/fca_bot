# Use latest stable n8n image
FROM n8nio/n8n:latest

# Set environment variables for n8n configuration
ENV NODE_ENV=production
ENV WEBHOOK_URL=${WEBHOOK_URL}
ENV N8N_ENCRYPTION_KEY=${N8N_ENCRYPTION_KEY}
ENV N8N_BASIC_AUTH_ACTIVE=true
ENV N8N_BASIC_AUTH_USER=${N8N_BASIC_AUTH_USER}
ENV N8N_BASIC_AUTH_PASSWORD=${N8N_BASIC_AUTH_PASSWORD}
ENV TELEGRAM_BOT_TOKEN=${TELEGRAM_BOT_TOKEN}
ENV GAPGPT_API_KEY=${GAPGPT_API_KEY}

# Default working directory in container
WORKDIR /data

# Expose the n8n port
EXPOSE 5678

# Start n8n
CMD ["n8n", "start"]
