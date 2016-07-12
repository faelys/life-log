/*
 * Copyright (c) 2016, Natacha Porté
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

const settings = {  /* "name in local storage": "form input parameter" */
   "event-list":    "ev",
   "begin-prefix":  "bpre",
   "end-prefix":    "epre",
};

function encodeStored(names) {
   var result = "?v=dev";
   for (var key in names) {
      var value = localStorage.getItem(key);
      if (value != null) {
         result = result + "&" + names[key] + "=" + encodeURIComponent(value);
      }
   }
   console.log("Encoded from storage: " + result);
   return result;
}

Pebble.addEventListener("ready", function() {
   console.log("Life-Log PebbleKit JS ready!");
});

Pebble.addEventListener("showConfiguration", function() {
   Pebble.openURL("https://cdn.rawgit.com/faelys/life-log/v1.0/config.html" + encodeStored(settings));
});

Pebble.addEventListener("webviewclosed", function(e) {
   var configData = JSON.parse(e.response);

   for (var key in settings) {
      localStorage.setItem(key, configData[key]);
   }

   const eventArray = configData["event-list"] !== "" ? configData["event-list"].split(",") : [];

   var dict = {
      1000: eventArray.length,
   };

   if (configData["begin-prefix"] !== null) {
      dict[901] = configData["begin-prefix"];
   }
   if (configData["end-prefix"] !== null) {
      dict[902] = configData["end-prefix"];
   }

   for (var i = 0; i < eventArray.length; i++) {
      dict[1001 + i] = eventArray[i];
   }

   Pebble.sendAppMessage(dict, function() {
      console.log("Send successful: " + JSON.stringify(dict));
   }, function() {
      console.log("Send failed!");
   });
});
