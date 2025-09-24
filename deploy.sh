#!/bin/bash

# Tank Duel Web Game - Google Cloud Run Deployment Script

set -e

# Configuration
PROJECT_ID="your-project-id"
SERVICE_NAME="tank-duel"
REGION="us-central1"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}🎮 Tank Duel Web Game - Cloud Run Deployment${NC}"
echo "=================================================="

# Check if gcloud is installed
if ! command -v gcloud &> /dev/null; then
    echo -e "${RED}❌ gcloud CLI is not installed. Please install it first.${NC}"
    exit 1
fi

# Check if user is authenticated
if ! gcloud auth list --filter=status:ACTIVE --format="value(account)" | grep -q .; then
    echo -e "${YELLOW}⚠️  Not authenticated with gcloud. Please run 'gcloud auth login'${NC}"
    exit 1
fi

# Prompt for project ID if not set
if [ "$PROJECT_ID" = "your-project-id" ]; then
    echo -e "${YELLOW}Please update PROJECT_ID in this script or set it as an environment variable${NC}"
    read -p "Enter your Google Cloud Project ID: " PROJECT_ID
fi

echo -e "${GREEN}📦 Building and deploying to project: $PROJECT_ID${NC}"

# Set the project
gcloud config set project $PROJECT_ID

# Enable required APIs
echo -e "${GREEN}🔧 Enabling required APIs...${NC}"
gcloud services enable cloudbuild.googleapis.com
gcloud services enable run.googleapis.com
gcloud services enable containerregistry.googleapis.com

# Build and deploy using Cloud Build
echo -e "${GREEN}🚀 Building and deploying with Cloud Build...${NC}"
gcloud builds submit --config cloudbuild.yaml

# Get the service URL
SERVICE_URL=$(gcloud run services describe $SERVICE_NAME --region=$REGION --format="value(status.url)")

echo ""
echo -e "${GREEN}✅ Deployment completed successfully!${NC}"
echo -e "${GREEN}🌐 Service URL: $SERVICE_URL${NC}"
echo ""
echo -e "${YELLOW}📋 Next steps:${NC}"
echo "1. Open the URL above to play Tank Duel"
echo "2. Share the URL with friends for multiplayer battles"
echo "3. Monitor your app: gcloud run services list"
echo ""
echo -e "${GREEN}🎯 Ready to battle! ${NC}"