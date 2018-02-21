#!/bin/bash
#

set -e

# prepare envoy config file.

cat > envoy.yaml << EOF
admin:
  access_log_path: /dev/stdout
  address:
    socket_address:
      address: 127.0.0.1
      port_value: 19000
static_resources:
  listeners:
  - name: listener_0
    address:
      socket_address: { address: 127.0.0.1, port_value: 10000 }
    filter_chains:
    - filters:
      - name: envoy.http_connection_manager
        config:
          stat_prefix: http
          codec_type: AUTO
          route_config:
            name: local_route
            virtual_hosts:
            - name: local_service
              domains: ["*"]
              routes:
              - match:
                  prefix: /post
                route:
                  cluster: my-nats-streaming
                metadata:
                  filter_metadata:
                      io.solo.function_router:
                        my-nats-streaming:
                          function: subject1
          http_filters:
          - name: io.solo.nats_streaming
            config:
              op_timeout: 5s
          - name: envoy.router
  clusters:
  - connect_timeout: 5.000s
    hosts:
    - socket_address:
        address: localhost
        port_value: 4222
    name: my-nats-streaming
    type: STRICT_DNS
    metadata:
      filter_metadata:
        io.solo.nats_streaming:
          discover_prefix: _STAN.discover
          cluster_id: test-cluster
EOF
