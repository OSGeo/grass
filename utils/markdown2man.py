import argparse
import sys
import re

def strip_yaml_from_markdown(content):
    # Remove YAML front matter
    return re.sub(r'^---\n.*?\n---\n', '', content, flags=re.DOTALL)


def parse_markdown(content):
    lines = content.splitlines()
    processing_block = []
    processed_content = []
    
    buffer = ""
    state = "default"
  
    
    for line in lines:
        if line.strip().startswith("```"):
            # end of code block
            if state == "code":
                processing_block.append(line)
                processed_content.append({"markdown": "\n".join(processing_block), "type": state})
                processing_block = []
                state = "default"
            # start of code block
            else:
                if buffer:
                    processing_block.append(buffer)
                    buffer = ""
                    processed_content.append({"markdown": "\n".join(processing_block), "type": state})
                    processing_block = []
                else:
                    processing_block.append(line)
                state = "code"
            continue

        if state == "code":
            processing_block.append(line)
            continue
        
        if line.endswith("  ") :
            buffer += line  # Keep trailing spaces for markdown line breaks
            processing_block.append(buffer)
            buffer = ""

        if line.strip().startswith("- ") or line.startswith("* "):
            
            if buffer:
                processing_block.append(buffer)
                buffer = ""
                
            # start of list
            if state != "list":
                processed_content.append({"markdown": "\n".join(processing_block), "type": state})
                processing_block = []
                state = "list"
            
            processing_block.append(line)  
            continue  
        
        if re.match(r"^\d+\.", line.strip()):
            if buffer:
                processing_block.append(buffer)
                buffer = ""
            # start of ordered list
            if state != "olist":
                processed_content.append({"markdown": "\n".join(processing_block), "type": state})
                processing_block = []
                state = "olist"
            
            processing_block.append(line)
            continue

        # empty line at the start and end of code, list blocks
        if line == "":
            if buffer:
                processing_block.append(buffer)
                buffer = ""
            if state != "default":
                processed_content.append({"markdown": "\n".join(processing_block), "type": state})
                processing_block = []
                state = "default"
            processing_block.append(line)
            continue

        buffer += line + " "
    
    if buffer:
        processing_block.append(buffer)
    if processing_block:
        processed_content.append({"markdown": "\n".join(processing_block), "type": state})
    
    return processed_content

def process_multiline_links(markdown):
    # Regular expression to match links that may span multiple lines
    link_pattern = re.compile(r'\[([^\]]*)\]\(([^)]+)\)', re.DOTALL)
    image_link_pattern = re.compile(r'!\[([^\]]*)\]\(([^)]+)\)', re.DOTALL)

    def replace_link(match):
        # Strip leading and trailing whitespace or newlines from the link URL and text
        alt_text = match.group(1).replace("\n", " ").strip()
        url = match.group(2).replace("\n", "").strip()
        return f'[{alt_text}]({url})'

    def replace_image_link(match):
        # Strip leading and trailing whitespace or newlines from the link URL and text
        alt_text = match.group(1).replace("\n", " ").strip()
        url = match.group(2).replace("\n", "").strip()
        return f'![{alt_text}]({url})'

    # Replace all matched links with the single-line version
    markdown = re.sub(link_pattern, replace_link, markdown)
    return re.sub(image_link_pattern, replace_image_link, markdown)

def process_markdown_formatting(md_text):
    # Regular expression to find multi-line formatting for bold, emphasis, and combined bold and emphasis
    pattern = r'(\*\*\*([^\*]+)\*\*\*|\*\*([^\*]+)\*\*|\*([^\*]+)\*)'

    def replace_match(match):
        # Match for combined bold and emphasis (***text***)
        if match.group(1).startswith('***'):
            content = match.group(2).replace('\n', ' ').strip()
            return f"***{content}***"
        
        # Match for bold (**text**)
        elif match.group(1).startswith('**'):
            content = match.group(3).replace('\n', ' ').strip()
            return f"**{content}**"
        
        # Match for emphasis (*text*)
        elif match.group(1).startswith('*'):
            content = match.group(4).replace('\n', ' ').strip()
            return f"*{content}*"
        
        return match.group(0)  # Return the original text if no match

    # Apply the regex pattern to replace formatting spans
    processed_text = re.sub(pattern, replace_match, md_text)

    return processed_text
  
def process_links(line):
    """Replace Markdown links with only their display text."""
    line = re.sub(r"!\[.*?\]\(.*?\)", "", line)
    return re.sub(r"\[(.*?)\]\((.*?)\)", r"\1", line)

def process_parameters(line):
    return re.sub(r"^\*\*([a-z0-9_]*)\*\*=\*([a-z]*)\*( \*\*\[required\]\*\*)?", r'.IP "**\1**=*\2*\3" 4m', line, flags=re.MULTILINE)

def process_flags(line):
    return re.sub(r"^\*\*-(.*?)\*\*", r'.IP "**-\1**" 4m', line, flags=re.MULTILINE)
  
def process_formatting(line):
    """Apply inline formatting for bold, italic, and bold+italic."""
    line = re.sub(r"\*\*\*(.+?)\*\*\*", r"\\fB\\fI\1\\fR", line)  # Bold+Italic
    line = re.sub(r"\*\*(.+?)\*\*", r"\\fB\1\\fR", line)  # Bold
    line = re.sub(r"\*(.+?)\*", r"\\fI\1\\fR", line)  # Italic
    line = line.replace("\u00A0", "    ")  # Replace non-breaking spaces with indent
    
    return line
  
def process_br(line):
    return re.sub(r"([^\n\s])  $", r"\1\n.br", line)

def process_headings(markdown):
    def convert_sh(match):
        return f".SH {match.group(1).upper()}\n"

    def convert_ss(match):
        return f".SS {match.group(1)}\n"

    markdown = re.sub(r"^#{1,2} (.*)", convert_sh, markdown, flags=re.MULTILINE)
    markdown = re.sub(r"^#{3,} (.*)", convert_ss, markdown, flags=re.MULTILINE)
    return markdown

def process_code(markdown):
    markdown = re.sub(r"\\", "\(rs", markdown)
    return markdown

def process_lists(markdown):
    markdown = re.sub(r"^(\d+)\.\s+(.*)", r'.IP \\fB\1\\fR\n\2\n', markdown, flags=re.MULTILINE)
    return re.sub(r"^(\d+)\.\s+(.*)", r'.IP \\fB\1\\fR\n\2\n', markdown, flags=re.MULTILINE)

def process_non_code(markdown):
    markdown_text = process_parameters(markdown)
    markdown_text = process_flags(markdown_text)
    markdown_text = markdown_text.replace("&nbsp;&nbsp;&nbsp;&nbsp;", "")
    
    markdown_text = re.sub(r"\\#", "#", markdown_text)
    markdown_text = re.sub(r"\\>", ">", markdown_text)
    markdown_text = re.sub(r"\\<", "<", markdown_text)
    markdown_text = re.sub(r"\\", "\(rs", markdown_text)
    markdown_text = re.sub(r"(?<=\S) {2,}(?=\S)", " ", markdown_text)
    markdown_text = re.sub(r'(?<=\S) {2,}(?=\S)', ' ', markdown_text)
    markdown_text = process_formatting(markdown_text)
    markdown_text = process_links(markdown_text)
    markdown_text = process_headings(markdown_text)
  
    return markdown_text
  
def convert_line(line, in_paragraph, in_code_block):
    """Convert a single line of Markdown to man page format, handling paragraph continuity."""
    if line.startswith("```"):
        if in_code_block:
            return "\\fR\n.fi\n", False, False  # End code block
        else:
            return ".nf\n\\fC\n", False, True  # Start code block with proper indent
    
    if in_code_block:
        return f"{line}\n", False, True  # Keep all whitespace in code blocks
    
    if not line.strip():
        return "", False, False  # Empty line resets paragraph state

    #line = process_headings(line)
    line = process_br(line)
    
    line = line.replace("`", "")

    if re.match(r"^[-*] (.+)", line):
        return f".IP \"{re.sub(r'^[-*] ', '', line)}\" 4m\n.br\n", False, False
    #if re.match(r"^\d+\. (.+)", line):
        #return re.sub(r"^(\d+)\.\s+(.*)", r'.IP \\fB\1\\fR\n\2\n', line, flags=re.MULTILINE), False, False
    if in_paragraph:
        return line + "\n", True, False
    else:
        return f".PP\n{line}\n", True, False


def markdown_to_man(markdown_text):
    """Convert a Markdown text to a Unix man page format"""
    markdown_text = strip_yaml_from_markdown(markdown_text)
    blocks = parse_markdown(markdown_text)
    result = []
    for block in blocks:
        if block["type"] == "code":
            result.append(process_code(block["markdown"]))
        elif block["type"] == "list":
            result.append(process_lists(block["markdown"]))
        elif block["type"] == "olist":
            result.append(process_lists(block["markdown"]))
        else:
            result.append(process_non_code(block["markdown"]))
    markdown_text = "\n".join(result)
    #markdown_text = process_parameters(markdown_text)
    #markdown_text = process_flags(markdown_text)
    #markdown_text = markdown_text.replace("&nbsp;&nbsp;&nbsp;&nbsp;", "")
    
    #markdown_text = re.sub(r"\\#", "#", markdown_text)
    #markdown_text = re.sub(r"\\>", ">", markdown_text)
    #markdown_text = re.sub(r"\\<", "<", markdown_text)
    #markdown_text = re.sub(r"\\", "\(rs", markdown_text)
    #markdown_text = process_formatting(markdown_text)
    #markdown_text = process_links(markdown_text)
    
    lines = markdown_text.splitlines()
    man_page = [".TH MAN 1 \"Manual\"\n"]
    in_paragraph = False
    in_code_block = False

    print(markdown_text)
    
    for line in lines:
        converted_line, in_paragraph, in_code_block = convert_line(line, in_paragraph, in_code_block)
        man_page.append(converted_line)
    
    if in_code_block:
        man_page.append(".fi\n")  # Ensure proper closure of code block
    
    return "".join(man_page)

def convert_markdown_to_man(input_file, output_file):
    """Read Markdown file and convert to man page."""
    with open(input_file, "r") as f:
        markdown_text = f.read()
    
    man_text = markdown_to_man(markdown_text)
    
    with open(output_file, "w") as f:
        f.write(man_text)
    
    print(f"Man page generated: {output_file}")

def main():
    parser = argparse.ArgumentParser(description="Convert Markdown to Unix man page.")
    parser.add_argument("input_file", help="Path to the input Markdown file.")
    parser.add_argument("output_file", help="Path to the output man page file.")
    args = parser.parse_args()
    
    convert_markdown_to_man(args.input_file, args.output_file)

if __name__ == "__main__":
    main()
