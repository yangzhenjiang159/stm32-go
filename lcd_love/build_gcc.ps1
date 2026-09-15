$ErrorActionPreference = 'Stop'
$root  = 'c:\Users\19833\CodeBuddy\20260914232137\lcd_love'
$tool  = 'c:\Users\19833\CodeBuddy\20260914232137\gcc-arm\gcc-arm-none-eabi-10.3-2021.10\bin'
$gcc   = Join-Path $tool 'arm-none-eabi-gcc.exe'
$oc    = Join-Path $tool 'arm-none-eabi-objcopy.exe'
$size  = Join-Path $tool 'arm-none-eabi-size.exe'

Set-Location -LiteralPath $root
New-Item -ItemType Directory -Force -Path 'Obj' | Out-Null

$common = @('-mcpu=cortex-m3', '-mthumb', '-std=gnu99', '-O2', '-ffunction-sections',
            '-fdata-sections', '-Wall', '-DSTM32F10X_HD',
            '-IGCC', '-IUser', '-IHardware\FSMC', '-IInterface\LCD')

function Run($exe, $argsList, $desc) {
    Write-Output ">>> $desc"
    & $exe @argsList 2>&1 | ForEach-Object { "$_" }
    if ($LASTEXITCODE -ne 0) { throw "$desc failed, exit=$LASTEXITCODE" }
}

Run $gcc (@('-x', 'assembler-with-cpp') + @('-c', 'GCC\startup_stm32f10x_hd.s', '-o', 'Obj\startup.o')) 'asm startup'

$srcs = @{
    'GCC\system_stm32f10x.c' = 'Obj\system_stm32f10x.o'
    'User\delay.c'           = 'Obj\delay.o'
    'User\main.c'            = 'Obj\main.o'
    'Hardware\FSMC\fsmc.c'   = 'Obj\fsmc.o'
    'Interface\LCD\lcd.c'    = 'Obj\lcd.o'
}
foreach ($s in $srcs.Keys) {
    Run $gcc ($common + @('-c', $s, '-o', $srcs[$s])) ('cc ' + $s)
}

$objs = @('Obj\startup.o', 'Obj\system_stm32f10x.o', 'Obj\delay.o',
          'Obj\main.o', 'Obj\fsmc.o', 'Obj\lcd.o')

Run $gcc (@('-mcpu=cortex-m3', '-mthumb', '-T', 'GCC\stm32_flash.ld',
            '-Wl,-Map=Obj\lcd_love.map', '-Wl,--gc-sections', '--specs=nosys.specs',
            '-o', 'Obj\lcd_love.elf') + $objs) 'link'

Run $oc @('-O', 'ihex', 'Obj\lcd_love.elf', 'Obj\lcd_love.hex') 'hex'
Run $size @('Obj\lcd_love.elf') 'size'

Write-Output 'BUILD OK'
