"""Locate the ONB (Austrian Books Online) copy of the 1517 Hospitaller breviary,
download its page images via IIIF and OCR them. Run on a GitHub runner."""
import json, os, re, subprocess, sys, time, urllib.parse, urllib.request
from concurrent.futures import ThreadPoolExecutor

OUT = "breviary/source"
UA = {"User-Agent": "breviary-research/1.0 (GitHub Actions; personal study)"}
os.makedirs(f"{OUT}/discovery", exist_ok=True)


def get(url, dest=None, tries=3):
    for i in range(tries):
        try:
            with urllib.request.urlopen(urllib.request.Request(url, headers=UA), timeout=60) as r:
                data = r.read()
            if dest:
                open(dest, "wb").write(data)
            return data
        except Exception as e:
            print(f"  ! {url}: {e}")
            time.sleep(3 * (i + 1))
    return b""


def candidates():
    # Known from ONB Primo record ONB_alma21289534790003338 (linktorsrc data.onb.ac.at/ABO/%2BZ221667307)
    found = {"+Z221667307"}
    sru = "https://obv-at-oenb.alma.exlibrisgroup.com/view/sru/43ACC_ONB?version=1.2&operation=searchRetrieve&maximumRecords=50&query="
    queries = ['alma.title="Breviarium secundum usum ordinis"',
               'alma.all_for_ui="Breviarium Hierosolymitani 1517"',
               'alma.all_for_ui="Breviarium Johannis Hierosolymitani"',
               'alma.all_for_ui="Hochperg 1517"']
    for n, q in enumerate(queries):
        d = get(sru + urllib.parse.quote(q), f"{OUT}/discovery/sru_{n}.xml").decode("utf-8", "replace")
        found |= set(re.findall(r"\+Z\d{6,}", d.replace("%2B", "+")))
    # Primo guest search
    tok = get("https://search.onb.ac.at/primo_library/libweb/webservices/rest/v1/guestJwt/ONB?isGuest=true&lang=de_DE&viewId=ONB")
    tok = tok.decode().strip('"') if tok else ""
    if tok:
        q = urllib.parse.quote("any,contains,Breviarium ordinis Johannis Hierosolymitani 1517")
        url = f"https://search.onb.ac.at/primo_library/libweb/webservices/rest/primo-explore/v1/pnxs?q={q}&vid=ONB&tab=default_tab&scope=ONB_gesamtbestand&limit=20&lang=de_DE"
        try:
            req = urllib.request.Request(url, headers={**UA, "Authorization": "Bearer " + tok})
            d = urllib.request.urlopen(req, timeout=60).read().decode("utf-8", "replace")
            open(f"{OUT}/discovery/primo.json", "w").write(d)
            found |= set(re.findall(r"\+Z\d{6,}", d.replace("%2B", "+")))
        except Exception as e:
            print("primo failed", e)
    return sorted(found)


def main():
    cands = candidates()
    print("barcodes:", cands)
    manifest = None
    for bc in cands:
        url = f"https://iiif.onb.ac.at/presentation/ABO/{bc}/manifest/"
        d = get(url)
        if not d:
            continue
        m = json.loads(d)
        label = json.dumps(m.get("label", "")) + json.dumps(m.get("metadata", ""))
        print(bc, label[:300])
        if re.search(r"(?i)breviar", label) and re.search(r"(?i)hierosolym|johann", label):
            manifest, barcode = m, bc
            break
    if not manifest:
        print("No matching manifest found")
        return
    json.dump(manifest, open(f"{OUT}/manifest.json", "w"), indent=1)
    open(f"{OUT}/barcode.txt", "w").write(barcode + "\n")
    canvases = manifest["sequences"][0]["canvases"]
    print(len(canvases), "canvases")
    os.makedirs("/tmp/img", exist_ok=True)
    os.makedirs(f"{OUT}/ocr", exist_ok=True)

    def work(args):
        i, c = args
        svc = c["images"][0]["resource"]["service"]["@id"].rstrip("/")
        img = f"/tmp/img/{i:04d}.jpg"
        if not get(f"{svc}/full/1600,/0/default.jpg", img):
            return
        subprocess.run(["tesseract", img, f"{OUT}/ocr/{i:04d}", "-l", "lat", "--psm", "4"],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        time.sleep(0.3)

    with ThreadPoolExecutor(4) as ex:
        list(ex.map(work, enumerate(canvases, 1)))
    with open(f"{OUT}/pages.tsv", "w") as f:
        for i, c in enumerate(canvases, 1):
            f.write(f"{i}\t{c.get('label','')}\t{c['images'][0]['resource']['service']['@id']}\n")


main()
