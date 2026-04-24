default:
	@@echo "NO MAIN FILE!"
test:
	@gcc -I./tests tests/parser_test.c tests/unity.c parser/http_parser.c parser/url.c -o tests/bin/parser_test 
	@gcc -I./tests tests/response_test.c tests/unity.c response/response_builder.c -o tests/bin/response_test 