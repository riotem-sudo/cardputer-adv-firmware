Add-Type -AssemblyName System.Drawing
$charsFile = 'C:\launcher\chars.txt'
$out = 'C:\launcher\Launcher-3.0.0\lib\DisplayDrivers\src\zh_font.h'
$preview = 'C:\launcher\font_preview.png'

$chars = [System.IO.File]::ReadAllText($charsFile, [System.Text.Encoding]::UTF8).Trim()
$list = @()
for ($i = 0; $i -lt $chars.Length; $i++) { $list += $chars[$i] }
"chars: $($list.Count)"

$W = 12; $H = 12
$cellPad = 0
$bigSize = 12
$previewScale = 4

$fontName = 'Microsoft YaHei UI'
$glyphs = @{}
$previewBmp = New-Object System.Drawing.Bitmap (8*(12*4+2)+4), ((([math]::Ceiling($list.Count/8))*($H*4+2)+4))
$pg = [System.Drawing.Graphics]::FromImage($previewBmp)
$pg.Clear([System.Drawing.Color]::White)

$idx = 0
foreach ($ch in $list) {
    # render big, then crop+threshold into 12x12
    $bmp = New-Object System.Drawing.Bitmap 32,32
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.Clear([System.Drawing.Color]::White)
    $g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::SingleBitPerPixelGridFit
    $f = New-Object System.Drawing.Font($fontName, $bigSize, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
    $sf = New-Object System.Drawing.StringFormat
    $sf.Alignment = [System.Drawing.StringAlignment]::Near
    $sf.LineAlignment = [System.Drawing.StringAlignment]::Near
    $g.DrawString([string]$ch, $f, [System.Drawing.Brushes]::Black, 2, 2, $sf)
    $f.Dispose(); $sf.Dispose(); $g.Dispose()

    # ink bbox
    $minX = 999; $minY = 999; $maxX = -1; $maxY = -1
    for ($y=0; $y -lt 32; $y++) {
      for ($x=0; $x -lt 32; $x++) {
        $c = $bmp.GetPixel($x,$y)
        if ((($c.R + $c.G + $c.B)/3) -lt 200) {
          if ($x -lt $minX) {$minX=$x}; if ($x -gt $maxX) {$maxX=$x}
          if ($y -lt $minY) {$minY=$y}; if ($y -gt $maxY) {$maxY=$y}
        }
      }
    }
    if ($maxX -lt 0) { $minX=0;$minY=0;$maxX=0;$maxY=0 }
    $bw = $maxX-$minX+1; $bh = $maxY-$minY+1
    $scale = [math]::Min(($W-$cellPad)/$bw, ($H-$cellPad)/$bh)
    if ($scale -gt 1) { $scale = 1 }
    if ($scale -lt 1) { $scale = 1 }
    $dw = [int][math]::Round($bw*$scale); $dh = [int][math]::Round($bh*$scale)
    $ox = [int][math]::Floor(($W-$dw)/2); $oy = [int][math]::Floor(($H-$dh)/2)

    # build 12 rows of 16-bit bitmaps (bits 11..0 = x 0..11)
    $rows = @()
    for ($ry=0; $ry -lt $H; $ry++) { $rows += 0 }
    for ($dy=0; $dy -lt $dh; $dy++) {
      $sy = $minY + [int][math]::Floor($dy/$scale)
      $bits = 0
      for ($dx=0; $dx -lt $dw; $dx++) {
        $sx = $minX + [int][math]::Floor($dx/$scale)
        $c = $bmp.GetPixel($sx,$sy)
        if ((($c.R + $c.G + $c.B)/3) -lt 170) {
          $px = $ox + $dx
          if ($px -ge 0 -and $px -lt $W) { $bits = $bits -bor (1 -shl (11 - $px)) }
        }
      }
      $ty = $oy + $dy
      if ($ty -ge 0 -and $ty -lt $H) { $rows[$ty] = $rows[$ty] -bor $bits }
    }
    $bmp.Dispose()
    $cp = [int][char]$ch
    $glyphs[$cp] = $rows

    # preview draw (enlarged)
    $col = $idx % 8; $row = [int][math]::Floor($idx/8)
    $cw = $W*$previewScale + 2
    $px0 = $col*$cw+1; $py0 = $row*($H*$previewScale+2)+1
    for ($yy=0; $yy -lt $H; $yy++) {
      $bits = $rows[$yy]
      for ($xx=0; $xx -lt $W; $xx++) {
        if (($bits -band (1 -shl (11-$xx))) -ne 0) {
          for ($a=0; $a -lt $previewScale; $a++) {
            for ($b=0; $b -lt $previewScale; $b++) {
              $previewBmp.SetPixel($px0+$xx*$previewScale+$b, $py0+$yy*$previewScale+$a, [System.Drawing.Color]::Black)
            }
          }
        }
      }
    }
    $idx++
}
$pg.Dispose()
$previewBmp.Save($preview, [System.Drawing.Imaging.ImageFormat]::Png)
$previewBmp.Dispose()

# write C++ header
$sb = New-Object System.Text.StringBuilder
[void]$sb.AppendLine('// Generated CJK bitmap font 12x12 - do not edit')
[void]$sb.AppendLine('#ifndef ZH_FONT_H')
[void]$sb.AppendLine('#define ZH_FONT_H')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('#define ZH_FONT_W 12')
[void]$sb.AppendLine('#define ZH_FONT_H 12')
[void]$sb.AppendLine('')
$cps = $glyphs.Keys | Sort-Object
[void]$sb.AppendLine("static const int ZH_GLYPH_COUNT = $($cps.Count);")
[void]$sb.AppendLine('static const uint16_t ZH_CP[] PROGMEM = {')
$line = ''
foreach ($cp in $cps) { $line += ("0x{0:X4}, " -f $cp); if ($line.Length -gt 90) { [void]$sb.AppendLine($line.TrimEnd()); $line='' } }
if ($line) { [void]$sb.AppendLine($line.TrimEnd()) }
[void]$sb.AppendLine('};')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('static const uint16_t ZH_BITS[][ZH_FONT_H] PROGMEM = {')
foreach ($cp in $cps) {
  $rows = $glyphs[$cp]
  $s = '  {'
  for ($i=0; $i -lt $rows.Count; $i++) { $s += ("0x{0:X3}" -f $rows[$i]); if ($i -lt $rows.Count-1) { $s += ',' } }
  $s += '},'
  [void]$sb.AppendLine($s)
}
[void]$sb.AppendLine('};')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('#endif')
[System.IO.File]::WriteAllText($out, $sb.ToString(), (New-Object System.Text.UTF8Encoding($false)))
"written: $out"
"preview: $preview"
