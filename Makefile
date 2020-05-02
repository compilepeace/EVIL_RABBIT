


TARGETS= evil_rabbit.so demo.so innocent
all: $(TARGETS)

evil_rabbit.so: evil_rabbit.c
	gcc -shared -fPIC $< -o $@ -ldl

demo.so: demo.c
	gcc -shared -fPIC $< -o $@ -ldl

innocent: innocent.c
	gcc $< -o $@

clean:
	rm $(TARGETS)
