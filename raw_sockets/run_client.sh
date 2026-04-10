echo "building client.cpp ..."

g++ client.cpp -o client.exe -std=c++23 -O3 -Wall -Wextra -pedantic

echo "successfully built client.cpp"
echo "running client.exe ..."

sudo ./client.exe