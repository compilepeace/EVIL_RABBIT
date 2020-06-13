TARGETS= evil_rabbit.so ./demo/demo.so ./demo/innocent
all: $(TARGETS)

evil_rabbit.so: evil_rabbit.c
	gcc -shared -fPIC $< -o $@ -ldl

./demo/demo.so: ./demo/demo.c
	gcc -shared -fPIC $< -o $@ -ldl

./demo/innocent: ./demo/innocent.c
	gcc $< -o $@

clean:
	rm $(TARGETS) /tmp/.snow_valley
