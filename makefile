build: clean
	mkdir -p build
	emcc *.c -o build/index.html

clean:
	rm -rf build/*
