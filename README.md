rvision
=====================

This is a C++ rvision test service.

## Releases
* Under `releases` folder

## Features
* two-way asynchronous rpc communication

## Protocol
See [schema](src/rpc/messages/rvision.proto)

## Config
See [config](config/rvision.json)

`logger_level : info/debug/error`
`logger_sink : console/file`

## Requirements
* A C++ compiler with C++20 support
* POCO
* boost
* Protobuf
* gRPC
* sqlite3
* gtest

## How to build

At the moment only `cmake-based` build systems are supported.


## How to run
cfg:

create `rvision.json` in app executable folder

app:

`./rvision`
