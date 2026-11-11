
FROM gcc:14-bookworm AS build

WORKDIR /app

COPY *.cpp ./

RUN g++ -std=c++17 -O2 -pthread -Wall -Wextra *.cpp -o PraxChat

FROM debian:bookworm-slim

WORKDIR /app

COPY --from=build /app/PraxChat .

ENV PORT=10000

CMD ["sh", "-c", "./PraxChat ${PORT:-10000}"]
