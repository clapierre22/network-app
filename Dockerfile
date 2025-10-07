FROM gcc:latest

WORKDIR /app
COPY user.c .
COPY user.h .

RUN gcc -pthread -o user user.c

EXPOSE 8000-8080

CMD ["./user"]
