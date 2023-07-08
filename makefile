build: clean
	mkdir build
	emcc *.c -o build/index.html

clean:
	rm -rf build
