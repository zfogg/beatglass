/* Add token spans to Hawkmoth signatures that are emitted as one flat name. */
(function () {
  function escapeHtml(value) {
    return value.replace(/[&<>"']/g, function (character) {
      return {"&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;"}[character];
    });
  }

  function decorateSignatures() {
    document.querySelectorAll(".sig .sig-name.descname").forEach(function (name) {
      if (name.querySelector(".n, .api-function-name, .api-keyword")) return;

      var text = name.textContent;
      var tokenPattern = /(~?[A-Za-z_]\w*)(?=\s*\()|\b(?:void|bool|class|enum|struct|virtual|public|private|protected|const|noexcept)\b/g;
      name.innerHTML = escapeHtml(text).replace(tokenPattern, function (token, functionName) {
        return '<span class="' + (functionName ? "api-function-name" : "api-keyword") + '">' + token + "</span>";
      });
    });
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", decorateSignatures);
  } else {
    decorateSignatures();
  }
}());
