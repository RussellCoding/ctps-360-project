default:
	@@echo "NO MAIN FILE!"
test:
	@gcc -I./tests/unity tests/parser_test.c tests/unity/unity.c parser/http_parser.c parser/url.c -o tests/bin/parser_test 
	@gcc -I./tests/unity tests/response_test.c tests/unity/unity.c response/response_builder.c -o tests/bin/response_test 
	@gcc -I./tests/unity tests/router_test.c tests/unity/unity.c parser/http_parser.c parser/url.c router/router.c router/handlers.c response/response_builder.c -o tests/bin/router_test 