# client.py
try:
    import urequests as requests
except ImportError:
    requests = None


def post_json(url: str, payload: dict, headers: dict | None = None, timeout_s: int = 8):
    """
    HTTP POST з JSON.
    ВАЖЛИВО: resp.close() обовʼязковий для MicroPython.
    """
    if requests is None:
        raise ImportError("urequests not available")

    resp = None
    try:
        resp = requests.post(url, json=payload, headers=headers)
        status = resp.status_code
        text = resp.text
        return status, text
    finally:
        if resp is not None:
            resp.close()


def get(url: str, headers: dict | None = None, timeout_s: int = 8):
    """
    HTTP GET.
    Повертає (status_code, text).
    """
    if requests is None:
        raise ImportError("urequests not available")

    resp = None
    try:
        resp = requests.get(url, headers=headers)
        status = resp.status_code
        text = resp.text
        return status, text
    finally:
        if resp is not None:
            resp.close()
            
def build_query(params: dict) -> str:
    """
    Формує query string без urllib.
    Значення приводимо до str.
    """
    parts = []
    for k, v in params.items():
        parts.append("{}={}".format(k, v))
    return "&".join(parts)


def get_with_params(base_url: str, params: dict, headers: dict | None = None):
    """
    GET з query params.
    """
    query = build_query(params)
    url = base_url + ("?" if "?" not in base_url else "&") + query
    return get(url, headers=headers)
