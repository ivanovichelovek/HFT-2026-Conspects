echo "building server.cpp ..."

g++ server.cpp -o server.exe -std=c++23 -O3 -pthread -lboost_system -Wall -Wextra -pedantic

echo "successfully built server.cpp"
echo "running server.exe ..."

./server.exe