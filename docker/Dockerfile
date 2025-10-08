FROM gcc:latest

RUN apt-get update && apt-get install -y cmake && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY src/ ./src/
COPY CMakeLists.txt

RUN cmake -S . -B build && cmake --build build

EXPOSE 8000-8080

CMD ["./build/user"]
