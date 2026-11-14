/* Add one narrow token span without disturbing Hawkmoth's generated markup. */
(function () {
  function decorateSignatures() {
    document.querySelectorAll(".sig .sig-name.descname .pre").forEach(function (token) {
      if (token.textContent !== "report_error" || token.parentElement.classList.contains("api-function-name")) return;
      var wrapper = document.createElement("span");
      wrapper.className = "api-function-name";
      while (token.firstChild) {
        wrapper.appendChild(token.firstChild);
      }
      token.appendChild(wrapper);
    });
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", decorateSignatures);
  } else {
    decorateSignatures();
  }
}());
