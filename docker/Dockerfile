FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    libboost-filesystem1.83.0 \
    libboost-program-options1.83.0 \
    libboost-thread1.83.0 \
    libdb5.3++t64 \
    libssl3t64 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /home/ubuntu/.coino && chown -R 1000:1000 /home/ubuntu/.coino

COPY bin/Coinod /usr/local/bin/Coinod
RUN chmod +x /usr/local/bin/Coinod

COPY docker/Coino.conf.example /home/ubuntu/.coino/Coino.conf.example

USER ubuntu
WORKDIR /home/ubuntu

EXPOSE 29293 29299

ENTRYPOINT ["/usr/local/bin/Coinod"]
CMD ["-datadir=/home/ubuntu/.coino", "-printtoconsole"]
