"""Follow-up jobs on the ONB scan (+Z221667307), driven by job.json.
- reocr: re-OCR every page with a historical-print Tesseract model into ocr_hist/
- image_pages: save readable JPEGs of the given page ranges into img/"""
import json, os, re, subprocess, time, urllib.request
from concurrent.futures import ThreadPoolExecutor

OUT = "breviary/source"
UA = {"User-Agent": "breviary-research/1.0 (GitHub Actions; personal study)"}
job = json.load(open("breviary/tools/job.json"))
pages = [l.split("\t") for l in open(f"{OUT}/pages.tsv").read().splitlines()]
svc = {int(p[0]): p[2].rstrip("/") for p in pages}


def get(url, dest=None):
    for i in range(3):
        try:
            with urllib.request.urlopen(urllib.request.Request(url, headers=UA), timeout=90) as r:
                data = r.read()
            if dest:
                open(dest, "wb").write(data)
            return data
        except Exception as e:
            print(f"  ! {url}: {e}")
            time.sleep(3 * (i + 1))
    return b""


def commit(msg):
    subprocess.run(["git", "add", OUT])
    if subprocess.run(["git", "commit", "-qm", msg]).returncode == 0:
        subprocess.run(["git", "pull", "-q", "--rebase"])
        subprocess.run(["git", "push", "-q"])


def model():
    tess = subprocess.run(["bash", "-c", "dirname $(find /usr/share -name lat.traineddata | head -1)"],
                          capture_output=True, text=True).stdout.strip()
    bases = ["https://ub-backup.bib.uni-mannheim.de/~stweil/tesstrain/GT4HistOCR/tessdata_best/",
             "https://ub-backup.bib.uni-mannheim.de/~stweil/tesstrain/frak2021/tessdata_best/"]
    for base in bases:
        listing = get(base).decode("utf-8", "replace")
        names = sorted(set(re.findall(r'href="([^"]+\.traineddata)"', listing)))
        print(base, names[-5:])
        if names:
            name = names[-1]
            if get(base + name, f"{tess}/hist.traineddata"):
                print("model:", base + name)
                return "hist"
    print("no historical model found; falling back to lat")
    return "lat"


os.makedirs("/tmp/img", exist_ok=True)

if job.get("image_pages"):
    os.makedirs(f"{OUT}/img", exist_ok=True)
    w = job.get("image_width", 1200)
    todo = [n for a, b in job["image_pages"] for n in range(a, b + 1)
            if not os.path.exists(f"{OUT}/img/{n:04d}.jpg")]
    q = job.get("image_quality")

    def fetch(n):
        dest = f"{OUT}/img/{n:04d}.jpg"
        if get(f"{svc[n]}/full/{w},/0/default.jpg", dest) and q:
            subprocess.run(["convert", dest, "-strip", "-quality", str(q), dest])

    with ThreadPoolExecutor(4) as ex:
        list(ex.map(fetch, todo))
    print(len(todo), "images saved")
    commit(f"Breviary page images: {job['image_pages']}")

if job.get("reocr"):
    lang = model()
    os.makedirs(f"{OUT}/ocr_hist", exist_ok=True)

    def work(n):
        if os.path.exists(f"{OUT}/ocr_hist/{n:04d}.txt"):
            return
        img = f"/tmp/img/{n:04d}.jpg"
        if get(f"{svc[n]}/full/2000,/0/default.jpg", img):
            subprocess.run(["tesseract", img, f"{OUT}/ocr_hist/{n:04d}", "-l", lang, "--psm", "4"],
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            os.remove(img)

    allp = sorted(svc)
    deadline = time.time() + 70 * 60
    with ThreadPoolExecutor(os.cpu_count() or 4) as ex:
        for b in range(0, len(allp), 150):
            if time.time() > deadline:
                print("Stopping before timeout; rerun to resume")
                break
            list(ex.map(work, allp[b:b + 150]))
            done = len(os.listdir(f"{OUT}/ocr_hist"))
            print(f"{done}/{len(allp)} re-OCRed ({lang})")
            commit(f"Breviary historical-model OCR: {done}/{len(allp)} pages ({lang})")
