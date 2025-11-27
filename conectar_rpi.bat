@echo off
echo ========================================
echo   CONECTAR AO RASPBERRY PI
echo ========================================
echo.
echo Tentando descobrir o IP do Raspberry Pi...
echo.

ping -n 1 raspberrypi.local

if %errorlevel% equ 0 (
    echo.
    echo ========================================
    echo   RASPBERRY PI ENCONTRADO!
    echo ========================================
    echo.
    echo Para conectar via SSH, use:
    echo   ssh pi@raspberrypi.local
    echo.
    echo Senha: [a senha que você configurou]
    echo.
) else (
    echo.
    echo ========================================
    echo   NÃO ENCONTRADO COM "raspberrypi.local"
    echo ========================================
    echo.
    echo Opção 1: Entre no roteador e veja os dispositivos conectados
    echo Opção 2: Use um scanner de rede (ex: Advanced IP Scanner)
    echo Opção 3: Conecte um monitor HDMI no Raspberry Pi
    echo.
)

echo.
echo Pressione qualquer tecla para sair...
pause > nul