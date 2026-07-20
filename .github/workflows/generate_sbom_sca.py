#!/usr/bin/env python3
import json
import subprocess
import os
import sys
import shutil
from datetime import date

# Paths
WORKSPACE = os.environ.get("GITHUB_WORKSPACE", os.getcwd())

def find_tool(name, default_fallback):
    path = shutil.which(name)
    if path:
        return path
    for p in [f"/opt/bin/{name}"]:
        if os.path.exists(p):
            return p
    return default_fallback

SYFT_PATH = find_tool("syft", "/opt/bin/syft")
GRYPE_PATH = find_tool("grype", "/opt/bin/grype")

SBOM_PATH = os.path.join(WORKSPACE, "sbom", "sbom.json")
CVE_PATH = os.path.join(WORKSPACE, "sbom", "cve-report.json")
SCA_JSON_PATH = os.path.join(WORKSPACE, "sbom", "sca-description.json")
SCA_HTML_PATH = os.path.join(WORKSPACE, "sbom", "sca-description.html")
MANIFEST_PATH = os.path.join(WORKSPACE, "document", "sbom", "manifest.json")

def load_manifest():
    if os.path.exists(MANIFEST_PATH):
        with open(MANIFEST_PATH, "r") as f:
            return json.load(f)
    return {}

def run_command(cmd, cwd=WORKSPACE):
    print(f"Running command: {' '.join(cmd)}")
    result = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"Command failed with code {result.returncode}")
        print(f"Stderr: {result.stderr}")
        sys.exit(1)
    return result.stdout

def main():
    manifest = load_manifest()
    project_name = "lv_port_nuvoton"
    supplier = manifest.get("supplier", "Nuvoton Technology")

    # Ensure output directory exists
    os.makedirs(os.path.dirname(SBOM_PATH), exist_ok=True)

    # 1. Run Syft to get base SBOM
    print("Step 1: Running Syft to generate base CycloneDX SBOM...")
    syft_cmd = [SYFT_PATH, "dir:" + WORKSPACE,
                "--exclude", "./.github/**",
                "--exclude", "./tools/**",
                "--exclude", "**/build/**",
                "-o", "cyclonedx-json"]
    base_sbom_str = run_command(syft_cmd)
    
    try:
        sbom = json.loads(base_sbom_str)
    except Exception as e:
        print(f"Failed to parse Syft output: {e}")
        sys.exit(1)
        
    # Ensure components list exists
    if "components" not in sbom:
        sbom["components"] = []

    # Normalize Syft-discovered paths: convert absolute workspace paths to relative
    ws_prefix = WORKSPACE.rstrip("/") + "/"
    for comp in sbom["components"]:
        name = comp.get("name", "")
        if name.startswith(ws_prefix):
            comp["name"] = name[len(ws_prefix):]
        for prop in comp.get("properties", []):
            val = prop.get("value", "")
            if val.startswith(ws_prefix):
                prop["value"] = val[len(ws_prefix):]
        
    # 2. Formulate and inject our custom third-party components from manifest.json
    print("Step 2: Injecting components from manifest.json into SBOM...")
    
    custom_components = []
    for comp in manifest.get("components", []):
        name = comp.get("name")
        version = comp.get("version")
        license_str = comp.get("license", "Unknown")
        purl = comp.get("purl")
        cpe = comp.get("cpe")
        description = comp.get("description", "")
        paths = comp.get("paths", [])
        
        licenses = []
        if license_str:
            if " " in license_str or "License" in license_str:
                licenses = [{"license": {"name": license_str}}]
            else:
                licenses = [{"license": {"id": license_str}}]
                
        properties = [
            {"name": "syft:package:foundBy", "value": "custom-thirdparty-cataloger"}
        ]
        for i, path in enumerate(paths):
            properties.append({"name": f"syft:location:{i}:path", "value": path})
            
        custom_components.append({
            "bom-ref": f"{purl}?package-id={name.replace('/', '-')}",
            "type": "library",
            "name": name,
            "version": version,
            "description": description,
            "purl": purl,
            "cpe": cpe,
            "licenses": licenses,
            "properties": properties
        })
    
    # Avoid duplicating if they are somehow already present
    existing_purls = {c.get("purl") for c in sbom["components"] if c.get("purl")}
    for cc in custom_components:
        if cc["purl"] not in existing_purls:
            sbom["components"].append(cc)
            
    # Update main metadata component representing the whole BSP project
    if "metadata" not in sbom:
        sbom["metadata"] = {}
    
    sbom["metadata"]["component"] = {
        "bom-ref": f"pkg:generic/{project_name}?type=firmware",
        "type": "firmware",
        "name": project_name,
        "supplier": {"name": supplier}
    }
    
    # Save the complete enriched SBOM
    with open(SBOM_PATH, "w") as f:
        json.dump(sbom, f, indent=2)
    print(f"Complete SBOM successfully written to {SBOM_PATH}")
    
    # 3. Run Grype scan on the enriched SBOM
    print("Step 3: Running Grype CVE scan on the enriched SBOM...")
    grype_cmd = [GRYPE_PATH, "sbom:" + SBOM_PATH, "-o", "json"]
    cve_report_str = run_command(grype_cmd)
    
    try:
        cve_report = json.loads(cve_report_str)
    except Exception as e:
        print(f"Failed to parse Grype output: {e}")
        sys.exit(1)
        
    with open(CVE_PATH, "w") as f:
        f.write(cve_report_str)
    print(f"Vulnerability report successfully written to {CVE_PATH}")
    
    # 4. Parse vulnerability details
    print("Step 4: Analyzing scan results and generating SCA description files...")
    vuln_summary = {
        "Critical": 0,
        "High": 0,
        "Medium": 0,
        "Low": 0,
        "Negligible": 0,
        "Unknown": 0
    }
    
    vulnerability_list = []
    
    matches = cve_report.get("matches", [])
    for match in matches:
        vulnerability = match.get("vulnerability", {})
        vuln_id = vulnerability.get("id")
        severity = vulnerability.get("severity", "Unknown").capitalize()
        if severity not in vuln_summary:
            severity = "Unknown"
        vuln_summary[severity] += 1
        
        artifact = match.get("artifact", {})
        pkg_name = artifact.get("name")
        pkg_version = artifact.get("version")
        pkg_purl = artifact.get("purl")
        
        related_vulns = match.get("relatedVulnerabilities", [])
        fix_state = "unknown"
        if related_vulns:
            fix_state = related_vulns[0].get("fix", {}).get("state", "unknown")

        vulnerability_list.append({
            "vuln_id": vuln_id,
            "severity": severity,
            "package_name": pkg_name,
            "package_version": pkg_version,
            "purl": pkg_purl,
            "description": vulnerability.get("description", "No description available."),
            "fix_state": fix_state
        })
        
    # Generate list of final components for the SCA description
    sca_components = []
    for c in sbom["components"]:
        licenses = []
        for lic_item in c.get("licenses", []):
            lic = lic_item.get("license", {})
            licenses.append(lic.get("id") or lic.get("name") or "Unknown")
            
        cve_count = sum(1 for v in vulnerability_list if v["package_name"] == c.get("name"))
        
        sca_components.append({
            "name": c.get("name"),
            "version": c.get("version"),
            "type": c.get("type"),
            "purl": c.get("purl"),
            "license": ", ".join(licenses) if licenses else "Unknown",
            "vulnerabilities_found": cve_count
        })
        
    # 5. Output SCA Description JSON
    generated_date = date.today().isoformat()
    sca_description = {
        "project_name": project_name,
        "generated_date": generated_date,
        "supplier": supplier,
        "total_components": len(sbom["components"]),
        "vulnerability_summary": vuln_summary,
        "components": sca_components,
        "vulnerabilities": vulnerability_list
    }
    
    with open(SCA_JSON_PATH, "w") as f:
        json.dump(sca_description, f, indent=2)
    print(f"SCA JSON description written to {SCA_JSON_PATH}")

    # 6. Output SCA Description HTML
    vuln_status_class = "status-fail" if vuln_summary['Critical'] + vuln_summary['High'] > 0 else "status-pass"
    vuln_status_text = "Action Required" if vuln_summary['Critical'] + vuln_summary['High'] > 0 else "Passed (No Critical/High Vulns)"

    html_components_rows = ""
    for c in sca_components:
        vuln_badge = f'<span class="badge badge-danger">{c["vulnerabilities_found"]}</span>' if c["vulnerabilities_found"] > 0 else '<span class="badge badge-ok">0</span>'
        display_name = c['name']
        html_components_rows += f"""        <tr>
          <td><strong>{display_name}</strong></td>
          <td><code>{c['version']}</code></td>
          <td>{c['type']}</td>
          <td>{c['license']}</td>
          <td>{vuln_badge}</td>
          <td><code>{c['purl'] or ''}</code></td>
        </tr>
"""

    severity_order = {"Critical": 0, "High": 1, "Medium": 2, "Low": 3, "Negligible": 4, "Unknown": 5}
    sorted_vulns = sorted(vulnerability_list, key=lambda x: severity_order.get(x["severity"], 99))

    html_vuln_rows = ""
    if not vulnerability_list:
        html_vuln_rows = '<tr><td colspan="6" class="no-vulns">No vulnerabilities were detected in the audited components.</td></tr>'
    else:
        for v in sorted_vulns:
            sev_class = v["severity"].lower()
            desc_short = v["description"][:200] if v["description"] else "N/A"
            html_vuln_rows += f"""        <tr>
          <td><span class="severity severity-{sev_class}">{v['severity']}</span></td>
          <td><code>{v['vuln_id']}</code></td>
          <td><strong>{v['package_name']}</strong></td>
          <td><code>{v['package_version']}</code></td>
          <td><code>{v['fix_state']}</code></td>
          <td>{desc_short}</td>
        </tr>
"""

    html_content = f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>SCA Report - {project_name}</title>
<style>
  :root {{
    --color-bg: #f0f2f5;
    --color-card: #ffffff;
    --color-border: #e0e0e0;
    --color-text: #1a1a2e;
    --color-muted: #6c757d;
    --color-accent: #4caf50;
    --color-critical: #dc3545;
    --color-high: #fd7e14;
    --color-medium: #ffc107;
    --color-low: #28a745;
    --color-negligible: #17a2b8;
    --color-unknown: #6c757d;
  }}
  * {{ box-sizing: border-box; margin: 0; padding: 0; }}
  body {{
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
    background: var(--color-bg);
    color: var(--color-text);
    line-height: 1.6;
    padding: 0;
  }}
  .header {{
    background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
    color: #fff;
    padding: 2rem 2rem 1.5rem;
  }}
  .header h1 {{ font-size: 1.6rem; font-weight: 700; letter-spacing: -0.02em; }}
  .header .subtitle {{ color: rgba(255,255,255,0.7); font-size: 0.95rem; margin-top: 0.25rem; }}
  .container {{ max-width: 1200px; margin: 0 auto; padding: 1.5rem 2rem 2rem; }}
  h2 {{
    font-size: 1.15rem;
    margin: 2rem 0 0.75rem;
    color: #1a1a2e;
    display: flex;
    align-items: center;
    gap: 0.5rem;
  }}
  h2::before {{
    content: '';
    display: inline-block;
    width: 4px;
    height: 1.15rem;
    background: var(--color-accent);
    border-radius: 2px;
  }}
  .card {{
    background: var(--color-card);
    border: 1px solid var(--color-border);
    border-radius: 10px;
    padding: 1.25rem;
    margin-bottom: 1.25rem;
    box-shadow: 0 1px 3px rgba(0,0,0,0.04);
  }}
  .summary-grid {{
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(160px, 1fr));
    gap: 1rem;
  }}
  .summary-item {{
    text-align: center;
    padding: 0.5rem;
  }}
  .summary-item .label {{
    font-size: 0.7rem;
    color: var(--color-muted);
    text-transform: uppercase;
    letter-spacing: 0.05em;
  }}
  .summary-item .value {{
    font-size: 1.3rem;
    font-weight: 700;
    display: block;
    margin-top: 0.15rem;
  }}
  .severity-grid {{
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(100px, 1fr));
    gap: 0.6rem;
  }}
  .severity-card {{
    text-align: center;
    padding: 0.6rem 0.4rem;
    border-radius: 8px;
    font-weight: 600;
    font-size: 0.8rem;
  }}
  .severity-card.critical {{ background: #f8d7da; color: var(--color-critical); }}
  .severity-card.high {{ background: #ffe5cc; color: #c35a00; }}
  .severity-card.medium {{ background: #fff3cd; color: #856404; }}
  .severity-card.low {{ background: #d4edda; color: #155724; }}
  .severity-card.negligible {{ background: #d1ecf1; color: #0c5460; }}
  .severity-card.unknown {{ background: #e2e3e5; color: #383d41; }}
  .severity-card .count {{ font-size: 1.5rem; display: block; line-height: 1.2; }}
  .status-pass {{ color: #155724; font-weight: 700; }}
  .status-fail {{ color: var(--color-critical); font-weight: 700; }}
  table {{
    width: 100%;
    border-collapse: separate;
    border-spacing: 0;
    font-size: 0.82rem;
  }}
  th, td {{
    padding: 0.55rem 0.7rem;
    text-align: left;
    border-bottom: 1px solid var(--color-border);
  }}
  th {{
    background: #f5f6f8;
    font-weight: 600;
    white-space: nowrap;
    position: sticky;
    top: 0;
    font-size: 0.75rem;
    text-transform: uppercase;
    letter-spacing: 0.03em;
    color: var(--color-muted);
  }}
  th:first-child {{ border-radius: 6px 0 0 0; }}
  th:last-child {{ border-radius: 0 6px 0 0; }}
  tr:hover {{ background: #f8f9fb; }}
  code {{
    background: #eef1f5;
    padding: 0.15rem 0.4rem;
    border-radius: 3px;
    font-size: 0.78rem;
    word-break: break-all;
  }}
  .badge {{
    padding: 0.2rem 0.55rem;
    border-radius: 10px;
    font-size: 0.72rem;
    font-weight: 600;
    display: inline-block;
    min-width: 1.5rem;
    text-align: center;
  }}
  .badge-danger {{ background: #f8d7da; color: var(--color-critical); }}
  .badge-ok {{ background: #d4edda; color: #155724; }}
  .severity {{
    padding: 0.2rem 0.55rem;
    border-radius: 4px;
    font-size: 0.72rem;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.02em;
  }}
  .severity-critical {{ background: #f8d7da; color: var(--color-critical); }}
  .severity-high {{ background: #ffe5cc; color: #c35a00; }}
  .severity-medium {{ background: #fff3cd; color: #856404; }}
  .severity-low {{ background: #d4edda; color: #155724; }}
  .severity-negligible {{ background: #d1ecf1; color: #0c5460; }}
  .severity-unknown {{ background: #e2e3e5; color: #383d41; }}
  .no-vulns {{ text-align: center; color: #155724; font-weight: 600; padding: 2rem; }}
  .footer {{
    margin-top: 2rem;
    padding-top: 1rem;
    border-top: 1px solid var(--color-border);
    text-align: center;
    color: var(--color-muted);
    font-size: 0.78rem;
  }}
  @media (max-width: 768px) {{
    .header {{ padding: 1.5rem 1rem; }}
    .container {{ padding: 1rem; }}
    .summary-grid {{ grid-template-columns: repeat(2, 1fr); }}
    .severity-grid {{ grid-template-columns: repeat(3, 1fr); }}
  }}
</style>
</head>
<body>
<div class="header">
  <div style="max-width:1200px;margin:0 auto;">
    <h1>Software Composition Analysis (SCA) Report</h1>
    <p class="subtitle">{project_name}</p>
  </div>
</div>
<div class="container">

  <div class="card">
    <div class="summary-grid">
      <div class="summary-item"><span class="label">Supplier</span><span class="value">{supplier}</span></div>
      <div class="summary-item"><span class="label">Generated Date</span><span class="value">{generated_date}</span></div>
      <div class="summary-item"><span class="label">Components</span><span class="value">{sca_description['total_components']}</span></div>
      <div class="summary-item"><span class="label">Status</span><span class="value {vuln_status_class}">{vuln_status_text}</span></div>
    </div>
  </div>

  <h2>Security Vulnerability Summary</h2>
  <div class="card">
    <div class="severity-grid">
      <div class="severity-card critical"><span class="count">{vuln_summary['Critical']}</span>Critical</div>
      <div class="severity-card high"><span class="count">{vuln_summary['High']}</span>High</div>
      <div class="severity-card medium"><span class="count">{vuln_summary['Medium']}</span>Medium</div>
      <div class="severity-card low"><span class="count">{vuln_summary['Low']}</span>Low</div>
      <div class="severity-card negligible"><span class="count">{vuln_summary['Negligible']}</span>Negligible</div>
      <div class="severity-card unknown"><span class="count">{vuln_summary['Unknown']}</span>Unknown</div>
    </div>
  </div>

  <h2>Component Composition</h2>
  <div class="card" style="overflow-x:auto;">
    <table>
      <thead>
        <tr><th>Name</th><th>Version</th><th>Type</th><th>License</th><th>Vulns</th><th>Package URL (purl)</th></tr>
      </thead>
      <tbody>
{html_components_rows}
      </tbody>
    </table>
  </div>

  <h2>Detailed Vulnerabilities</h2>
  <div class="card" style="overflow-x:auto;">
    <table>
      <thead>
        <tr><th>Severity</th><th>ID</th><th>Component</th><th>Version</th><th>Fix Status</th><th>Description</th></tr>
      </thead>
      <tbody>
{html_vuln_rows}
      </tbody>
    </table>
  </div>

  <div class="footer">
    <p><a href="sbom.json" style="color:var(--color-accent);text-decoration:none;">Download SBOM (sbom.json)</a></p>
    <p>Generated by lv_port_nuvoton SBOM/SCA pipeline on {generated_date}</p>
  </div>
</div>
</body>
</html>
"""

    with open(SCA_HTML_PATH, "w", encoding="utf-8") as f:
        f.write(html_content)
    print(f"SCA HTML report successfully written to {SCA_HTML_PATH}")

    # 7. Pack reports into a single ZIP file
    print("Step 7: Packaging all generated reports into sca-reports.zip...")
    import zipfile
    zip_path = os.path.join(WORKSPACE, "sca-reports.zip")
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zipf:
        for filepath in [SBOM_PATH, CVE_PATH, SCA_JSON_PATH, SCA_HTML_PATH]:
            arcname = os.path.basename(filepath)
            zipf.write(filepath, arcname)
    print(f"Zip bundle successfully written to {zip_path}")
    print("\n🎉 SBOM and SCA generation completed successfully!")

if __name__ == "__main__":
    main()
