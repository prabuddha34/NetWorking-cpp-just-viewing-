
FROM gcc:14-bookworm

WORKDIR /app

COPY *.cpp ./

RUN g++ -std=c++17 -O2 -pthread -Wall -Wextra *.cpp -o PraxChat

ENV PORT=10000

EXPOSE 10000

CMD ["sh", "-c", "./PraxChat ${PORT:-10000}"]
