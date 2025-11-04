FROM gcc:latest

WORKDIR /app
COPY user.c .
COPY user.h .
COPY util.h .
COPY packet.h .
COPY packet.c .

RUN gcc -pthread -o user user.c packet.c

EXPOSE 8000-8080

CMD ["./user"]
