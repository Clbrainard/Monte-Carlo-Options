import os
import re
import csv
import json
import time
import math
from datetime import datetime, timezone
from pathlib import Path

import google.generativeai as genai
from playwright.sync_api import sync_playwright


# =========================================================
# CONFIG
# =========================================================

os.getenv("GEMINI_API_KEY")
if not GEMINI_API_KEY:
    raise RuntimeError("Set GEMINI_API_KEY in your environment first.")

genai.configure(api_key=GEMINI_API_KEY)

MODEL_NAME = "gemini-1.5-pro"   # or "gemini-1.5-flash"
OUTPUT_CSV = "yahoo_options_extracted.csv"
# =========================================================
# HELPERS
# =========================================================
def yahoo_options_url(ticker: str) -> str:
    ticker = ticker.strip().upper()
    return f"https://finance.yahoo.com/quote/{ticker}/options/"


def get_rendered_html(url: str, wait_ms: int = 5000) -> str:
    """
    Use Playwright because Yahoo Finance is often dynamically rendered.
    """
    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True)
        page = browser.new_page()
        page.goto(url, wait_until="networkidle", timeout=60000)
        page.wait_for_timeout(wait_ms)
        html = page.content()
        browser.close()
        return html


def get_epoch_expiry_from_url(url: str) -> int | None:
    """
    Yahoo options pages sometimes include ?date=UNIX_TIMESTAMP.
    """
    m = re.search(r"[?&]date=(\d+)", url)
    if m:
        return int(m.group(1))
    return None


def minutes_until_expiry(expiry_epoch: int | None) -> int | None:
    if expiry_epoch is None:
        return None
    now = datetime.now(timezone.utc).timestamp()
    return max(0, int((expiry_epoch - now) // 60))


def chunk_text(text: str, chunk_size: int = 50000, overlap: int = 3000):
    """
    Split large HTML into overlapping chunks so Gemini can process it safely.
    """
    chunks = []
    start = 0
    n = len(text)
    while start < n:
        end = min(start + chunk_size, n)
        chunks.append(text[start:end])
        if end == n:
            break
        start = end - overlap
    return chunks


def extract_rows_with_gemini(
    html_chunk: str,
    ticker: str,
    fallback_minutes_until_expiry: int | None = None
):
    """
    Ask Gemini to extract ONLY rows with the requested fields.
    We ask for JSON for easy downstream CSV writing.
    """
    model = genai.GenerativeModel(MODEL_NAME)

    prompt = f"""
You are extracting option chain data from Yahoo Finance HTML.

Ticker: {ticker}

Return ONLY valid JSON in this exact format:
{{
  "rows": [
    {{
      "stockPrice": number_or_null,
      "strike": number_or_null,
      "optPrice": number_or_null,
      "volatility": number_or_null,
      "minutesUntilExpiry": number_or_null,
      "moneyness": number_or_null,
      "divYield": number_or_null
    }}
  ]
}}

Field rules:
- stockPrice = underlying stock price shown on page, repeated for each option row
- strike = strike price
- optPrice = option last price or market price
- volatility = implied volatility as a decimal, not percent
  Example: 25.31% -> 0.2531
- minutesUntilExpiry = use expiry info from page if available; otherwise use this fallback: {fallback_minutes_until_expiry}
- moneyness = stockPrice / strike
- divYield = dividend yield as decimal, not percent
  Example: 0.92% -> 0.0092

Important:
- Extract rows from the options chain only.
- Ignore missing rows rather than inventing data.
- Do not include commentary.
- Do not include markdown fences.
- Return JSON only.

HTML:
{html_chunk}
""".strip()

    response = model.generate_content(prompt)
    text = response.text.strip()

    # Strip accidental markdown fences if Gemini adds them anyway
    text = re.sub(r"^```json\s*", "", text)
    text = re.sub(r"^```\s*", "", text)
    text = re.sub(r"\s*```$", "", text)

    data = json.loads(text)
    rows = data.get("rows", [])
    return rows


def dedupe_rows(rows):
    seen = set()
    out = []
    for r in rows:
        key = (
            r.get("stockPrice"),
            r.get("strike"),
            r.get("optPrice"),
            r.get("volatility"),
            r.get("minutesUntilExpiry"),
            r.get("moneyness"),
            r.get("divYield"),
        )
        if key not in seen:
            seen.add(key)
            out.append(r)
    return out


def write_csv(rows, path):
    fieldnames = [
        "stockPrice",
        "strike",
        "optPrice",
        "volatility",
        "minutesUntilExpiry",
        "moneyness",
        "divYield",
    ]
    with open(path, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


# =========================================================
# MAIN PIPELINE
# =========================================================
def extract_yahoo_option_data(ticker: str):
    url = yahoo_options_url(ticker)
    print(f"Fetching: {url}")

    html = get_rendered_html(url)

    # Save raw HTML so you can inspect/debug it
    raw_html_path = f"{ticker.upper()}_options_page.html"
    Path(raw_html_path).write_text(html, encoding="utf-8")
    print(f"Saved raw HTML to: {raw_html_path}")

    fallback_expiry_epoch = get_epoch_expiry_from_url(url)
    fallback_minutes = minutes_until_expiry(fallback_expiry_epoch)

    chunks = chunk_text(html, chunk_size=50000, overlap=3000)

    all_rows = []
    for i, chunk in enumerate(chunks, start=1):
        print(f"Processing chunk {i}/{len(chunks)} with Gemini...")
        try:
            rows = extract_rows_with_gemini(
                html_chunk=chunk,
                ticker=ticker,
                fallback_minutes_until_expiry=fallback_minutes
            )
            all_rows.extend(rows)
        except Exception as e:
            print(f"Chunk {i} failed: {e}")

    all_rows = dedupe_rows(all_rows)
    write_csv(all_rows, OUTPUT_CSV)

    print(f"Done. Extracted {len(all_rows)} rows.")
    print(f"CSV saved to: {OUTPUT_CSV}")


ticker = "MSFT"   # replace with your string variable
extract_yahoo_option_data(ticker)