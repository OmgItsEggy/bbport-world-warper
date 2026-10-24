import html.parser
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SITE = ROOT / "docs"
REPOSITORY_URL = "https://github.com/OmgItsEggy/bbport-world-warper"


class SiteParser(html.parser.HTMLParser):
    def __init__(self):
        super().__init__()
        self.ids = set()
        self.links = []
        self.stylesheets = []
        self.title_parts = []
        self._in_title = False

    def handle_starttag(self, tag, attrs):
        values = dict(attrs)
        if "id" in values:
            self.ids.add(values["id"])
        if tag == "a" and "href" in values:
            self.links.append(values["href"])
        if tag == "link" and values.get("rel") == "stylesheet":
            self.stylesheets.append(values.get("href"))
        if tag == "title":
            self._in_title = True

    def handle_endtag(self, tag):
        if tag == "title":
            self._in_title = False

    def handle_data(self, data):
        if self._in_title:
            self.title_parts.append(data)


class ProjectSiteTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.html = (SITE / "index.html").read_text(encoding="utf-8")
        cls.parser = SiteParser()
        cls.parser.feed(cls.html)

    def test_required_sections_and_title_exist(self):
        self.assertEqual("".join(self.parser.title_parts).strip(), "bbport World Warper")
        self.assertTrue({"main", "top", "coverage", "architecture", "roadmap"} <= self.parser.ids)

    def test_repository_links_are_consistent(self):
        repository_links = [link for link in self.parser.links if link.startswith("https://github.com/")]
        self.assertGreaterEqual(len(repository_links), 3)
        self.assertEqual(set(repository_links), {REPOSITORY_URL})

    def test_site_is_honest_about_release_status(self):
        self.assertIn("Prototype only; no downloadable build yet.", self.html)
        self.assertIn("Early development", self.html)

    def test_stylesheets_are_relative_and_present(self):
        self.assertEqual(self.parser.stylesheets, ["assets/site.css"])
        for stylesheet in self.parser.stylesheets:
            self.assertTrue((SITE / stylesheet).is_file())


if __name__ == "__main__":
    unittest.main()
