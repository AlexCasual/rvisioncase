FROM ubuntu as build

RUN echo "Acquire::Check-Valid-Until \"false\";\nAcquire::Check-Date \"false\";" | cat > /etc/apt/apt.conf.d/10no--check-valid-until

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y

RUN apt install build-essential manpages-dev software-properties-common -y
RUN add-apt-repository ppa:ubuntu-toolchain-r/test -y
RUN apt update && apt install gcc-11 g++-11 -y

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 110 \
--slave /usr/bin/g++ g++ /usr/bin/g++-11 \
--slave /usr/bin/gcov gcov /usr/bin/gcov-11 \
--slave /usr/bin/gcc-ar gcc-ar /usr/bin/gcc-ar-11 \
--slave /usr/bin/gcc-ranlib gcc-ranlib /usr/bin/gcc-ranlib-11

RUN apt-get install -y \
    openssh-server \
    gdb \
	pkg-config \
	libsqlite3-0 \
    rsync zip make cmake ninja-build \
    libpoco-dev libboost-dev libgtest-dev libgrpc-dev libgrpc++-dev protobuf-compiler-grpc\
    protobuf-compiler libprotoc-dev libprotobuf-dev

ENV CMAKE_C_COMPILER=/usr/bin/gcc
ENV CMAKE_CXX_COMPILER=/usr/bin/g++
ENV CMAKE_PROTOBUF_COMPILER=/usr/bin/protoc

WORKDIR /root

RUN mkdir rvision && mkdir rvision/src && mkdir rvision/contrib && mkdir rvision/config

COPY ./docker/CMakeLists.txt rvision
COPY ./src rvision/src
COPY ./contrib rvision/contrib
COPY ./config rvision/config

# expected executable path: /root/build/rvision/src
RUN mkdir build && cd build && cmake ../rvision && make

FROM ubuntu as run

RUN echo "Acquire::Check-Valid-Until \"false\";\nAcquire::Check-Date \"false\";" | cat > /etc/apt/apt.conf.d/10no--check-valid-until

ENV DEBIAN_FRONTEND=noninhteractive

RUN apt-get update -y

RUN apt install build-essential manpages-dev software-properties-common -y
RUN add-apt-repository ppa:ubuntu-toolchain-r/test -y
RUN apt update && apt install gcc-11 g++-11 -y

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 110 \
--slave /usr/bin/g++ g++ /usr/bin/g++-11 \
--slave /usr/bin/gcov gcov /usr/bin/gcov-11 \
--slave /usr/bin/gcc-ar gcc-ar /usr/bin/gcc-ar-11 \
--slave /usr/bin/gcc-ranlib gcc-ranlib /usr/bin/gcc-ranlib-11

RUN apt-get update && apt-get install -y \
    libpocoutil62 \
	libpoconet62 \
	libsqlite3-0 \
	libgrpc6 \
	libgrpc++1 \
	libstdc++6

WORKDIR /root

# Copy binary from build container
COPY --from=build /root/rvision/build .

# Run service
CMD ["./bin/rvision"]
