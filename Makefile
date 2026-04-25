SRC = parser/http_parser.c parser/url.c response/response_builder.c router/handlers.c router/router.c socket/connection.c
TESTING_SRC = $(SRC) tests/unity/unity.c

default:
	@@echo "NO MAIN FILE!"
test:
	@gcc -I./tests/unity tests/parser_test.c $(TESTING_SRC) -o tests/bin/parser_test 
	@gcc -I./tests/unity tests/response_test.c $(TESTING_SRC) -o tests/bin/response_test 
	@gcc -I./tests/unity tests/router_test.c $(TESTING_SRC) -o tests/bin/router_test 
	@gcc -I./tests/unity tests/connection_test.c $(TESTING_SRC) -o tests/bin/connection_test 