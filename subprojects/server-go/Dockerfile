# Stage 1: Build binary
FROM golang:1.22-alpine AS builder

WORKDIR /app

# Copy go module files first for caching
COPY go.mod ./
RUN go mod download

# Copy source files
COPY . .

# Build lightweight CGO-free static binary
RUN CGO_ENABLED=0 GOOS=linux go build -ldflags="-w -s" -o nst-server-go .

# Stage 2: Minimal runtime environment (~12MB total size)
FROM alpine:latest

RUN apk --no-cache add ca-certificates

WORKDIR /root/

COPY --from=builder /app/nst-server-go .

EXPOSE 14478 8080

CMD ["./nst-server-go"]
