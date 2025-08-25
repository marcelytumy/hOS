#!/usr/bin/env python3
import sys
import os

def convert_bmp_to_header(bmp_file, output_file):
    """Convert a BMP file to a C++ header file with embedded data."""
    
    try:
        with open(bmp_file, 'rb') as f:
            bmp_data = f.read()
    except FileNotFoundError:
        print(f"Error: Could not find BMP file '{bmp_file}'")
        return False
    except Exception as e:
        print(f"Error reading BMP file: {e}")
        return False
    
    # Get the filename without extension for the variable name
    base_name = os.path.splitext(os.path.basename(bmp_file))[0]
    var_name = f"{base_name}_bmp"
    size_var_name = f"{base_name}_bmp_size"
    
    # Create the header file content
    header_content = f"""// Auto-generated from {bmp_file}
// Do not edit manually

#pragma once
#include <cstdint>

extern const uint8_t {var_name}[];
extern const uint32_t {size_var_name};
"""
    
    # Create the implementation file content
    impl_content = f"""// Auto-generated from {bmp_file}
// Do not edit manually

#include "{base_name}.hpp"

const uint8_t {var_name}[] = {{
"""
    
    # Add the BMP data as hex bytes
    for i, byte in enumerate(bmp_data):
        if i % 16 == 0:
            impl_content += "    "
        impl_content += f"0x{byte:02x}"
        if i < len(bmp_data) - 1:
            impl_content += ", "
        if i % 16 == 15 or i == len(bmp_data) - 1:
            impl_content += "\n"
    
    impl_content += f"""}};

const uint32_t {size_var_name} = {len(bmp_data)};
"""
    
    # Write the header file
    try:
        with open(output_file, 'w') as f:
            f.write(header_content)
        print(f"Created header file: {output_file}")
    except Exception as e:
        print(f"Error writing header file: {e}")
        return False
    
    # Write the implementation file
    impl_file = output_file.replace('.hpp', '.cpp')
    try:
        with open(impl_file, 'w') as f:
            f.write(impl_content)
        print(f"Created implementation file: {impl_file}")
    except Exception as e:
        print(f"Error writing implementation file: {e}")
        return False
    
    return True

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 convert_bmp.py <bmp_file> <output_header>")
        print("Example: python3 convert_bmp.py assets/logo.bmp src/logo.hpp")
        return 1
    
    bmp_file = sys.argv[1]
    output_file = sys.argv[2]
    
    if not convert_bmp_to_header(bmp_file, output_file):
        return 1
    
    print("Conversion completed successfully!")
    return 0

if __name__ == "__main__":
    sys.exit(main())
