# Use multi-stage build
FROM debian:12 AS build

WORKDIR /building

RUN apt-get update -y && \
    apt-get upgrade -y && \
    apt-get install -y build-essential libtcmalloc-minimal4 git cmake libasio-dev libboost-all-dev nlohmann-json3-dev g++ libcurl4-openssl-dev libgnutls28-dev libgsasl7-dev libz-dev libiconv-hook-dev libssl-dev libsasl2-dev zlib1g-dev doxygen msmtp libmysqlcppconn-dev && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* && \
    ln -s /usr/lib/libtcmalloc_minimal.so.4 /usr/lib/libtcmalloc_minimal.so && \
    git clone https://github.com/CrowCpp/Crow.git && \
    git clone https://github.com/Thalhammer/jwt-cpp.git && \
    git clone https://github.com/kisli/vmime.git && \
    git clone https://github.com/cpp-redis/cpp_redis.git

WORKDIR /building/Crow
RUN mkdir build
WORKDIR /building/Crow/build
RUN cmake .. -DASIO_INCLUDE_DIR=/usr/include -DCROW_BUILD_EXAMPLES=OFF -DCROW_BUILD_TESTS=OFF && \
    make install

WORKDIR /building/jwt-cpp
RUN cmake . && \
    cmake --build . && \
    cmake --install .

WORKDIR /building/vmime
RUN mkdir build
WORKDIR /building/vmime/build
    RUN cmake .. -DVMIME_SENDMAIL_PATH=/usr/bin/msmtp && \
        cmake --build . && \
        make install


WORKDIR /building/cpp_redis
RUN git submodule init && git submodule update && \
    mkdir build && \
    cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make && \
    make install

WORKDIR /building/libbcrypt
RUN git clone https://github.com/trusch/libbcrypt && \
cd libbcrypt && \
mkdir build && \
cd build && \
cmake .. && \
make && \
make install && \
ldconfig

WORKDIR /app

COPY . .
RUN mkdir build
WORKDIR /app/build
RUN cmake .. && \
    make

# Runtime stage
FROM debian:12 AS runtime

WORKDIR /app

RUN apt-get update -y && \
    apt-get install -y libssl-dev libcurl4 libgnutls28-dev libgsasl7-dev libz-dev libmysqlcppconn7v5 && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

COPY --from=build /app/build/BackEnd .
COPY --from=build /app/settings .

EXPOSE 45801

HEALTHCHECK --interval=5m --timeout=3s \
  CMD curl -f http://localhost:45801/ || exit 1

CMD ["./BackEnd"]