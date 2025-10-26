#!/usr/bin/env python3
"""
RS300 Media Controller Topology Visualizer
Generates PNG diagrams from media controller topology for troubleshooting
"""

import subprocess
import sys
import os
import re
import argparse
from typing import Dict, List, Tuple, Optional

class MediaEntity:
    def __init__(self, entity_id: int, name: str, entity_type: str, device_node: str = ""):
        self.entity_id = entity_id
        self.name = name
        self.entity_type = entity_type
        self.device_node = device_node
        self.pads = {}  # pad_id -> MediaPad
        self.links = []  # List of MediaLink

class MediaPad:
    def __init__(self, pad_id: int, direction: str, format_info: str = ""):
        self.pad_id = pad_id
        self.direction = direction  # "Source" or "Sink"
        self.format_info = format_info
        self.links = []  # List of links from/to this pad

class MediaLink:
    def __init__(self, source_entity: str, source_pad: int, sink_entity: str, sink_pad: int, status: str):
        self.source_entity = source_entity
        self.source_pad = source_pad
        self.sink_entity = sink_entity
        self.sink_pad = sink_pad
        self.status = status  # "ENABLED", "DISABLED", or ""

class MediaTopologyParser:
    def __init__(self, media_device: str = "/dev/media0"):
        self.media_device = media_device
        self.entities = {}  # entity_id -> MediaEntity
        self.links = []  # List of MediaLink
        
    def parse_topology(self) -> bool:
        """Parse media-ctl topology output"""
        try:
            result = subprocess.run(
                ["media-ctl", "-d", self.media_device, "--print-topology"],
                capture_output=True, text=True, check=True
            )
            topology_text = result.stdout
        except subprocess.CalledProcessError as e:
            print(f"Error running media-ctl: {e}")
            return False
        except FileNotFoundError:
            print("Error: media-ctl not found. Install v4l-utils package.")
            return False
            
        return self._parse_topology_text(topology_text)
    
    def _parse_topology_text(self, text: str) -> bool:
        """Parse the topology text output"""
        lines = text.split('\n')
        current_entity = None
        current_pad = None
        
        for line in lines:
            line = line.strip()
            
            # Parse entity line: "- entity 1: csi2 (8 pads, 9 links)"
            entity_match = re.match(r'- entity (\d+): (.+?) \((\d+) pads?, (\d+) links?\)', line)
            if entity_match:
                entity_id = int(entity_match.group(1))
                entity_name = entity_match.group(2)
                current_entity = MediaEntity(entity_id, entity_name, "unknown")
                self.entities[entity_id] = current_entity
                continue
                
            # Parse entity type line: "type V4L2 subdev subtype Sensor flags 0"
            if line.startswith("type ") and current_entity:
                current_entity.entity_type = line
                continue
                
            # Parse device node line: "device node name /dev/v4l-subdev2"
            if line.startswith("device node name ") and current_entity:
                current_entity.device_node = line.replace("device node name ", "")
                continue
                
            # Parse pad line: "pad0: Sink" or "pad4: Source"
            pad_match = re.match(r'pad(\d+): (Source|Sink)', line)
            if pad_match and current_entity:
                pad_id = int(pad_match.group(1))
                direction = pad_match.group(2)
                current_pad = MediaPad(pad_id, direction)
                current_entity.pads[pad_id] = current_pad
                continue
                
            # Parse format line: "[fmt:YUYV8_2X8/640x512 field:none ...]"
            if line.startswith("[fmt:") and current_pad:
                current_pad.format_info = line
                continue
                
            # Parse link line: "-> "rp1-cfe-csi2_ch0":0 [ENABLED]" or "<- "rs300 10-003c":0 [ENABLED,IMMUTABLE]"
            link_match = re.match(r'(->|<-) "([^"]+)":(\d+) \[([^\]]*)\]', line)
            if link_match and current_entity and current_pad:
                direction_arrow = link_match.group(1)
                target_entity = link_match.group(2)
                target_pad = int(link_match.group(3))
                status = link_match.group(4)
                
                if direction_arrow == "->":
                    # This pad is source
                    link = MediaLink(current_entity.name, current_pad.pad_id, target_entity, target_pad, status)
                else:
                    # This pad is sink
                    link = MediaLink(target_entity, target_pad, current_entity.name, current_pad.pad_id, status)
                
                self.links.append(link)
                current_pad.links.append(link)
                
        return True

class GraphvizGenerator:
    def __init__(self, parser: MediaTopologyParser):
        self.parser = parser
        
    def generate_dot(self, focus_rs300: bool = True) -> str:
        """Generate DOT format graph"""
        dot = ['digraph MediaTopology {']
        dot.append('  rankdir=LR;')
        dot.append('  node [shape=box, style=rounded];')
        dot.append('  edge [fontsize=10];')
        dot.append('')
        
        # Add subgraph for legend
        dot.append('  subgraph cluster_legend {')
        dot.append('    label="Status Legend";')
        dot.append('    style=filled;')
        dot.append('    color=lightgrey;')
        dot.append('    legend_enabled [label="ENABLED", style=filled, fillcolor=lightgreen];')
        dot.append('    legend_disabled [label="DISABLED", style=filled, fillcolor=lightcoral];')
        dot.append('    legend_unknown [label="No Status", style=filled, fillcolor=lightyellow];')
        dot.append('  }')
        dot.append('')
        
        # Generate nodes for entities
        for entity_id, entity in self.parser.entities.items():
            node_name = self._sanitize_name(entity.name)
            
            # Determine node color based on entity type and importance
            if "rs300" in entity.name.lower():
                color = "lightblue"
                shape = "doubleoctagon"
            elif "csi2" in entity.name.lower():
                color = "lightcyan"
                shape = "hexagon"
            elif "rp1-cfe" in entity.name.lower():
                color = "lightgreen"
                shape = "box"
            elif entity.device_node.startswith("/dev/video"):
                color = "lightyellow"
                shape = "oval"
            else:
                color = "white"
                shape = "box"
                
            # Create label with entity info
            label_parts = [entity.name]
            if entity.device_node:
                label_parts.append(f"({entity.device_node})")
                
            # Add format information for key pads
            if "rs300" in entity.name.lower() or "csi2" in entity.name.lower():
                for pad_id, pad in entity.pads.items():
                    if pad.format_info and "fmt:" in pad.format_info:
                        format_match = re.search(r'fmt:([^/\s]+)/([^/\s]+)', pad.format_info)
                        if format_match:
                            fmt = format_match.group(1)
                            resolution = format_match.group(2)
                            label_parts.append(f"pad{pad_id}: {fmt}")
                            label_parts.append(f"({resolution})")
                            
            label = "\\n".join(label_parts)
            
            dot.append(f'  {node_name} [label="{label}", shape={shape}, style=filled, fillcolor={color}];')
            
        dot.append('')
        
        # Generate edges for links
        for link in self.parser.links:
            source_name = self._sanitize_name(link.source_entity)
            sink_name = self._sanitize_name(link.sink_entity)
            
            # Determine edge color and style based on status
            if "ENABLED" in link.status:
                color = "green"
                style = "solid"
                penwidth = "2"
            elif "DISABLED" in link.status:
                color = "red"
                style = "dashed"
                penwidth = "1"
            else:
                color = "gray"
                style = "dotted"
                penwidth = "1"
                
            # Create edge label
            edge_label = f"pad{link.source_pad}→pad{link.sink_pad}"
            if link.status:
                edge_label += f"\\n[{link.status}]"
                
            dot.append(f'  {source_name} -> {sink_name} [label="{edge_label}", color={color}, style={style}, penwidth={penwidth}];')
            
        dot.append('}')
        return '\n'.join(dot)
    
    def _sanitize_name(self, name: str) -> str:
        """Sanitize entity name for DOT format"""
        # Replace problematic characters
        sanitized = re.sub(r'[^a-zA-Z0-9_]', '_', name)
        # Ensure it starts with a letter
        if sanitized and sanitized[0].isdigit():
            sanitized = 'entity_' + sanitized
        return sanitized or 'unknown'

class MediaVisualizer:
    def __init__(self, media_device: str = "/dev/media0"):
        self.media_device = media_device
        self.parser = MediaTopologyParser(media_device)
        
    def find_rs300_media_device(self) -> Optional[str]:
        """Find the media device containing RS300"""
        for i in range(6):
            test_device = f"/dev/media{i}"
            if not os.path.exists(test_device):
                continue
                
            try:
                result = subprocess.run(
                    ["media-ctl", "-d", test_device, "--print-topology"],
                    capture_output=True, text=True, check=True
                )
                if "rs300 10-003c" in result.stdout:
                    return test_device
            except subprocess.CalledProcessError:
                continue
                
        return None
        
    def generate_visualization(self, output_file: str = "media_topology.png", 
                            check_formats: bool = False, show_links: bool = False) -> bool:
        """Generate visualization PNG"""
        
        # Auto-detect RS300 media device if not specified
        if self.media_device == "/dev/media0":
            auto_device = self.find_rs300_media_device()
            if auto_device:
                self.media_device = auto_device
                self.parser.media_device = auto_device
                print(f"Auto-detected RS300 on {auto_device}")
            else:
                print("Warning: RS300 not found in any media device")
                
        if not self.parser.parse_topology():
            print("Failed to parse media topology")
            return False
            
        generator = GraphvizGenerator(self.parser)
        dot_content = generator.generate_dot()
        
        # Write DOT file
        dot_file = output_file.replace('.png', '.dot')
        try:
            with open(dot_file, 'w') as f:
                f.write(dot_content)
            print(f"DOT file written to {dot_file}")
        except IOError as e:
            print(f"Error writing DOT file: {e}")
            return False
            
        # Generate PNG using system graphviz
        try:
            result = subprocess.run(
                ["dot", "-Tpng", dot_file, "-o", output_file],
                capture_output=True, text=True, check=True
            )
            print(f"PNG visualization saved to {output_file}")
            
            # Clean up DOT file unless debugging
            if not show_links:  # Keep DOT file for debugging if requested
                os.remove(dot_file)
                
        except subprocess.CalledProcessError as e:
            print(f"Error generating PNG with graphviz: {e}")
            print("Make sure graphviz is installed: sudo apt install graphviz")
            return False
        except FileNotFoundError:
            print("Error: 'dot' command not found. Install graphviz: sudo apt install graphviz")
            return False
            
        # Additional analysis if requested
        if check_formats:
            self._check_format_compatibility()
            
        if show_links:
            self._show_link_status()
            
        return True
        
    def _check_format_compatibility(self):
        """Check format compatibility across pipeline"""
        print("\n=== Format Compatibility Check ===")
        
        rs300_format = None
        csi2_formats = {}
        
        for entity_id, entity in self.parser.entities.items():
            if "rs300" in entity.name.lower():
                for pad_id, pad in entity.pads.items():
                    if pad.direction == "Source" and pad.format_info:
                        format_match = re.search(r'fmt:([^/\s]+)', pad.format_info)
                        if format_match:
                            rs300_format = format_match.group(1)
                            print(f"RS300 output format: {rs300_format}")
                            
            elif "csi2" in entity.name.lower():
                for pad_id, pad in entity.pads.items():
                    if pad.format_info:
                        format_match = re.search(r'fmt:([^/\s]+)', pad.format_info)
                        if format_match:
                            csi2_formats[pad_id] = format_match.group(1)
                            print(f"CSI2 pad{pad_id} format: {format_match.group(1)}")
                            
        # Check for format mismatches
        if rs300_format:
            for pad_id, fmt in csi2_formats.items():
                if fmt != rs300_format and fmt != "unknown":
                    print(f"⚠️  Format mismatch: RS300({rs300_format}) != CSI2 pad{pad_id}({fmt})")
                    
    def _show_link_status(self):
        """Show detailed link status"""
        print("\n=== Link Status Analysis ===")
        
        enabled_links = []
        disabled_links = []
        unknown_links = []
        
        for link in self.parser.links:
            link_desc = f"{link.source_entity}:pad{link.source_pad} -> {link.sink_entity}:pad{link.sink_pad}"
            
            if "ENABLED" in link.status:
                enabled_links.append((link_desc, link.status))
            elif "DISABLED" in link.status:
                disabled_links.append((link_desc, link.status))
            else:
                unknown_links.append((link_desc, link.status or "No status"))
                
        print(f"✅ Enabled links ({len(enabled_links)}):")
        for link_desc, status in enabled_links:
            print(f"  {link_desc} [{status}]")
            
        if disabled_links:
            print(f"\n❌ Disabled links ({len(disabled_links)}):")
            for link_desc, status in disabled_links:
                print(f"  {link_desc} [{status}]")
                
        if unknown_links:
            print(f"\n❓ Unknown status links ({len(unknown_links)}):")
            for link_desc, status in unknown_links:
                print(f"  {link_desc} [{status}]")

def main():
    parser = argparse.ArgumentParser(description="RS300 Media Controller Topology Visualizer")
    parser.add_argument("--device", "-d", default="/dev/media0", 
                       help="Media controller device (default: auto-detect)")
    parser.add_argument("--output", "-o", default="media_topology.png",
                       help="Output PNG file (default: media_topology.png)")
    parser.add_argument("--check-formats", action="store_true",
                       help="Check format compatibility across pipeline")
    parser.add_argument("--show-links", action="store_true", 
                       help="Show detailed link status information")
    parser.add_argument("--all-formats", action="store_true",
                       help="Generate visualizations for all common format combinations")
    
    args = parser.parse_args()
    
    visualizer = MediaVisualizer(args.device)
    
    if args.all_formats:
        # Generate multiple format visualizations
        formats = ["current", "YUYV8_2X8", "YUYV8_1X16"]
        for fmt in formats:
            output_file = f"media_topology_{fmt}.png"
            print(f"\nGenerating visualization for {fmt}...")
            if visualizer.generate_visualization(output_file, args.check_formats, args.show_links):
                print(f"Success: {output_file}")
            else:
                print(f"Failed: {output_file}")
    else:
        success = visualizer.generate_visualization(args.output, args.check_formats, args.show_links)
        if success:
            print(f"\n✅ Visualization complete: {args.output}")
        else:
            print("\n❌ Visualization failed")
            sys.exit(1)

if __name__ == "__main__":
    main()