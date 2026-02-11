all:
	gcc -o fuck_discord fuck_discord.c -Wall -Wextra -Werror -O3 -m64 -s -flto -ffunction-sections -fdata-sections -Wl,--gc-sections -lpsapi
