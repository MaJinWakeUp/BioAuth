#!/bin/bash

# Default experiment type
EXPERIMENT_TYPE="secure-authentication"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --experiment|-e)
            EXPERIMENT_TYPE="$2"
            shift 2
            ;;
        --help|-h)
            echo "Usage: $0 [--experiment|-e EXPERIMENT_TYPE]"
            echo ""
            echo "Available experiments:"
            echo "  secure-authentication  Complete authentication process (default)"
            echo "  dot-product-db        Dot product database operations"
            echo "  dot-product-db-offline  Dot product database offline operations" 
            echo "  secure-com            Secure comparison operations"
            echo ""
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

echo "BioAuth Network Simulation Experiment: $EXPERIMENT_TYPE"

case $EXPERIMENT_TYPE in
    "secure-authentication")
        docker build -f Dockerfile.simple -t bioauth . && docker run --rm --privileged bioauth
        ;;
    "dot-product-db")
        docker build -f Dockerfile.dot-product-db -t bioauth-dotprod . && docker run --rm --privileged bioauth-dotprod
        ;;
    "dot-product-db-offline")
        docker build -f Dockerfile.dot-product-db-offline -t bioauth-dotprod-offline . && docker run --rm --privileged bioauth-dotprod-offline
        ;;
    "secure-com")
        docker build -f Dockerfile.secure-com -t bioauth-seccom . && docker run --rm --privileged bioauth-seccom
        ;;
    *)
        echo "Error: Unknown experiment type '$EXPERIMENT_TYPE'"
        echo "Use --help to see available experiments"
        exit 1
        ;;
esac