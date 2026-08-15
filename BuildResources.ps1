# PowerShell script to auto-generate C++ resource files from asset folders
$ErrorActionPreference = "Stop"

$projectDir = $PSScriptRoot
if (-not $projectDir) { $projectDir = Get-Location }

$rcHeaderFile = Join-Path $projectDir "generated_resources.h"
$rcFile = Join-Path $projectDir "generated_resources.rc"

Write-Host "Auto-generating resources in $projectDir..."

$headerContent = @"
#pragma once

#define IDI_ICON1 1

#define IDR_KNOPKA2_PNG        501
#define IDR_KNOPKA_NAJATA2_PNG 502
#define IDR_FAKECLOSEPHOTO_PNG 503
#define IDR_BSOD_JPG           504

// ID Ranges:
// Screamer Images:  1000 - 1999
// Screamer Sounds:  2000 - 2999
// Flashbang Sounds: 3000 - 3999
// Long Sounds:      4000 - 4999
// Captcha Images:   5000 - 5999
"@

Set-Content -Path $rcHeaderFile -Value $headerContent -Encoding UTF8

$rcLines = [System.Collections.Generic.List[string]]::new()
$rcLines.Add('#include "generated_resources.h"')
$rcLines.Add('')
$rcLines.Add('IDI_ICON1 ICON "icon.ico"')
$rcLines.Add('')

# 1. Screamer Images (RCDATA) -> ID 1000+
$screamerPicsDir = Join-Path $projectDir "screamerpics"
$rcLines.Add('// ==========================================')
$rcLines.Add('// Screamer Images (RCDATA)')
$rcLines.Add('// ==========================================')
if (Test-Path $screamerPicsDir) {
    $imgFiles = Get-ChildItem -Path $screamerPicsDir -File | Where-Object { $_.Extension -match '\.(png|jpg|jpeg|bmp|webp)$' } | Sort-Object Name
    $id = 1000
    foreach ($file in $imgFiles) {
        $relPath = "screamerpics\\" + $file.Name
        $rcLines.Add("$id RCDATA `"$relPath`"")
        $id++
    }
}
$rcLines.Add('')

# 2. Screamer Sounds (WAVE) -> ID 2000+
$screamerSoundsDir = Join-Path $projectDir "screamersounds"
$rcLines.Add('// ==========================================')
$rcLines.Add('// Screamer Sounds (WAVE)')
$rcLines.Add('// ==========================================')
if (Test-Path $screamerSoundsDir) {
    $sndFiles = Get-ChildItem -Path $screamerSoundsDir -File | Where-Object { $_.Extension -match '\.wav$' } | Sort-Object Name
    $id = 2000
    foreach ($file in $sndFiles) {
        $relPath = "screamersounds\\" + $file.Name
        $rcLines.Add("$id WAVE `"$relPath`"")
        $id++
    }
}
$rcLines.Add('')

# 3. Flashbang Sounds (WAVE) -> ID 3000+
$flashbangsDir = Join-Path $projectDir "flashbangs"
$rcLines.Add('// ==========================================')
$rcLines.Add('// Flashbang Sounds (WAVE)')
$rcLines.Add('// ==========================================')
if (Test-Path $flashbangsDir) {
    $fbFiles = Get-ChildItem -Path $flashbangsDir -File | Where-Object { $_.Extension -match '\.wav$' } | Sort-Object Name
    $id = 3000
    foreach ($file in $fbFiles) {
        $relPath = "flashbangs\\" + $file.Name
        $rcLines.Add("$id WAVE `"$relPath`"")
        $id++
    }
}
$rcLines.Add('')

# 4. Long Sounds (WAVE) -> ID 4000+
$longSoundsDir = Join-Path $projectDir "longsounds"
$rcLines.Add('// ==========================================')
$rcLines.Add('// Long Sounds (WAVE)')
$rcLines.Add('// ==========================================')
if (Test-Path $longSoundsDir) {
    $lsFiles = Get-ChildItem -Path $longSoundsDir -File | Where-Object { $_.Extension -match '\.wav$' } | Sort-Object Name
    $id = 4000
    foreach ($file in $lsFiles) {
        $relPath = "longsounds\\" + $file.Name
        $rcLines.Add("$id WAVE `"$relPath`"")
        $id++
    }
}
$rcLines.Add('')

# 5. Captcha Images (RCDATA) -> ID 5000+
$kapchaDir = Join-Path $projectDir "kapcha"
$rcLines.Add('// ==========================================')
$rcLines.Add('// Captcha Images (RCDATA)')
$rcLines.Add('// ==========================================')
if (Test-Path $kapchaDir) {
    $kapchaFiles = Get-ChildItem -Path $kapchaDir -File | Where-Object { $_.Extension -match '\.(png|jpg|jpeg|bmp|webp)$' } | Sort-Object Name
    $id = 5000
    foreach ($file in $kapchaFiles) {
        $relPath = "kapcha\\" + $file.Name
        $rcLines.Add("$id RCDATA `"$relPath`"")
        $id++
    }
}
$rcLines.Add('')

# 6. Buttons & Special Photos (RCDATA)
$rcLines.Add('// ==========================================')
$rcLines.Add('// Buttons & Special Photos (RCDATA)')
$rcLines.Add('// ==========================================')
$knopka1 = "C:\\Users\\dispair\\Desktop\\knopka2.psd.png"
$knopka2 = "C:\\Users\\dispair\\Desktop\\knopka_najata2.psd.png"
$fakeCloseFile = Join-Path $projectDir "fakeclosephoto.png"
$bsodFile = Join-Path $projectDir "bsod.jpg"

if (Test-Path $knopka1) {
    $rcLines.Add("IDR_KNOPKA2_PNG RCDATA `"$knopka1`"")
}
if (Test-Path $knopka2) {
    $rcLines.Add("IDR_KNOPKA_NAJATA2_PNG RCDATA `"$knopka2`"")
}
if (Test-Path $fakeCloseFile) {
    $rcLines.Add("IDR_FAKECLOSEPHOTO_PNG RCDATA `"fakeclosephoto.png`"")
}
if (Test-Path $bsodFile) {
    $rcLines.Add("IDR_BSOD_JPG RCDATA `"bsod.jpg`"")
}

Set-Content -Path $rcFile -Value $rcLines -Encoding UTF8
Write-Host "Resources successfully generated: $rcFile"
