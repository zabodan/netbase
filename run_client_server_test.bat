set target=release

start "server" cmd /c "%target%\netbase_server.exe 170 > logs\_server.log"

for /L %%i in (1,1,100) do (
  start "client %%i" cmd /c "%target%\netbase_client.exe 200 > logs\_client_%%i.log"
)