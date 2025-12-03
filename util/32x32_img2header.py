#!/usr/bin/env python3
"""
32x32 RGB565 컬러 아이콘 헤더 변환기
- 모든 크기의 이미지를 32x32로 스케일링
- RGB565 포맷으로 변환 (65,536색)
- C/C++ 헤더 파일 생성 (2048바이트)
"""

import sys
import os
from PIL import Image
import argparse

def rgb_to_rgb565(r, g, b):
    """RGB (8,8,8) 값을 RGB565 (5,6,5) 포맷으로 변환"""
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F  
    b5 = (b >> 3) & 0x1F
    return (r5 << 11) | (g6 << 5) | b5

def image_to_rgb565_header(image_path, output_path=None, icon_name=None):
    """
    모든 크기의 이미지를 32x32 RGB565 컬러 비트맵 C 헤더 파일로 변환
    
    Args:
        image_path: 입력 이미지 경로 (모든 크기 지원)
        output_path: 출력 헤더 파일 경로 (선택사항)
        icon_name: 아이콘 이름 (선택사항)
    """
    
    try:
        # 이미지 열기
        img = Image.open(image_path)
        original_size = img.size
        
        # RGBA 모드로 변환
        img = img.convert('RGBA')
        
        # 32x32로 리사이즈 (항상 수행)
        print(f"이미지 크기를 {original_size}에서 (32, 32)로 조정합니다.")
        img = img.resize((32, 32), Image.Resampling.LANCZOS)
        
        # 파일명에서 아이콘 이름 추출
        if not icon_name:
            base_name = os.path.splitext(os.path.basename(image_path))[0]
            icon_name = base_name.lower().replace(' ', '_').replace('-', '_')
        
        # 출력 파일 경로 설정
        if not output_path:
            output_dir = os.path.dirname(image_path) or '.'
            output_path = os.path.join(output_dir, f"{icon_name}_32x32.h")
        
        # RGB565 컬러 데이터 생성
        color_data = []
        
        for row in range(32):
            for col in range(32):
                pixel = img.getpixel((col, row))
                
                if len(pixel) >= 4:
                    r, g, b, a = pixel
                    if a < 128:  # 투명한 픽셀은 검은색
                        r, g, b = 0, 0, 0
                else:
                    r, g, b = pixel[:3]
                
                rgb565 = rgb_to_rgb565(r, g, b)
                color_data.append(rgb565)
        
        # 헤더 파일 생성
        header_guard = f"{icon_name.upper()}_32X32_H"
        array_name = f"{icon_name}_32x32"
        
        header_content = f"""#ifndef {header_guard}
#define {header_guard}

#include <stdint.h>

// 32x32 픽셀 {icon_name} 컬러 아이콘 (RGB565, 2048바이트)
// 원본 크기: {original_size[0]}x{original_size[1]}
static const uint16_t PROGMEM {array_name}[] = {{"""
        
        # 데이터 추가 (8개씩 한 줄)
        for i, color in enumerate(color_data):
            if i % 8 == 0:
                if i > 0:
                    # 행 번호 주석 추가
                    header_content += f",  // Row {(i-8)//32}\n" if i % 32 == 8 else ",\n"
                header_content += f"\n    0x{color:04X}"
            else:
                header_content += f", 0x{color:04X}"
        
        header_content += f"   // Row 31\n}};\n\n#endif // {header_guard}\n"
        
        # 파일 저장
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(header_content)
        
        print(f"✅ 변환 완료!")
        print(f"   입력: {image_path} ({original_size[0]}x{original_size[1]})")
        print(f"   출력: {output_path}")
        print(f"   배열명: {array_name}")
        print(f"   크기: 2048바이트 (32x32 RGB565)")
        print(f"\n📋 사용법:")
        print(f"   1. 헤더 파일을 include 폴더에 복사")
        print(f"   2. #include \"{os.path.basename(output_path)}\" 추가")
        print(f"   3. drawRGBBitmap(x, y, {array_name}, 32, 32); 로 사용")
        
        return True
        
    except Exception as e:
        print(f"❌ 오류 발생: {e}")
        import traceback
        traceback.print_exc()
        return False

def main():
    parser = argparse.ArgumentParser(
        description="모든 크기의 이미지를 32x32 RGB565 컬러 비트맵 C 헤더 파일로 변환",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
사용 예시:
  python 32x32_img2header.py icon.png
  python 32x32_img2header.py logo.jpg -o output.h -n my_logo
  python 32x32_img2header.py *.png  (여러 파일 일괄 변환)

출력:
  - 32x32 픽셀 RGB565 포맷 (2048바이트)
  - 65,536색 지원
  - PROGMEM 배열로 생성

요구사항:
  - PIL(Pillow) 라이브러리: pip install Pillow
        """
    )
    
    parser.add_argument('images', nargs='+', help='변환할 이미지 파일들 (모든 크기 지원)')
    parser.add_argument('-o', '--output', help='출력 헤더 파일 경로 (단일 파일만)')
    parser.add_argument('-n', '--name', help='아이콘 이름 (배열명에 사용, 단일 파일만)')
    
    args = parser.parse_args()
    
    # PIL 설치 확인
    try:
        from PIL import Image
    except ImportError:
        print("❌ PIL(Pillow) 라이브러리가 필요합니다.")
        print("   설치: pip install Pillow")
        return 1
    
    success_count = 0
    
    for image_path in args.images:
        if not os.path.exists(image_path):
            print(f"❌ 파일을 찾을 수 없습니다: {image_path}")
            continue
        
        print(f"\n🎨 RGB565 변환 중: {image_path}")
        
        # 여러 파일일 경우 output/name 옵션 무시
        output_path = args.output if len(args.images) == 1 else None
        icon_name = args.name if len(args.images) == 1 else None
        
        success = image_to_rgb565_header(image_path, output_path, icon_name)
        
        if success:
            success_count += 1
    
    print(f"\n🎉 완료: {success_count}/{len(args.images)} 파일 변환됨")
    return 0 if success_count > 0 else 1

if __name__ == "__main__":
    exit(main())
