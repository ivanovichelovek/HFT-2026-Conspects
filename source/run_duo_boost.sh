#!/usr/bin/env bash

set -e

if [ "$#" -ne 2 ]; then
    echo "Ошибка: нужно передать ровно 2 аргумента: source_ip dest_ip" >&2
    echo "Пример: ./run_duo_boost.sh 10.200.1.1 10.200.1.2" >&2
    exit 1
fi

source_ip="$1"
dest_ip="$2"

echo "Compiling client_boost_asio.cpp..."
g++ client_boost_asio.cpp -o client_boost_asio -std=c++23 -O3 -pthread -lboost_system

echo "Running client..."
sudo ./client_boost_asio "$source_ip" "$dest_ip"