d3.csv("http://localhost:8000/source/info.txt", function(data) {
  var size = +data[0].size;
  var countOfThreads = 5;
  var kThreadDistance = 50;

  var rankDistance = kThreadDistance*2;
  var maxTime = +data[0].maxTime;

  var widthOfRanks = (rankDistance + kThreadDistance*countOfThreads)* size;
  // set the dimensions and margins of the graph
  var margin = {top: 10, right: 10, bottom: 10, left: 10},
    width = widthOfRanks - margin.left - margin.right,
    height = maxTime - margin.top - margin.bottom;

  var startX = 100, startY = 10;

  var recv = [];
  var send = [];
  var barrier = [];
  var dup = [];
  var wait = [];
  var reduce = [];
  var messages = [];
  // append the svg object to the body of the page
  var svg = d3.select("#main")
  .append("svg")
    .attr("width", width + margin.left + margin.right)
    .attr("height", height + margin.top + margin.bottom)
  .append("g")
    .attr("transform",
          "translate(" + margin.left + "," + margin.top + ")");

  var kTime = Math.ceil(maxTime/100);
  for (var i = 1; i < kTime; i++) {     
    var y = startY + 100*i;
    svg.append("line")
      .attr("x1", 0)
      .attr("x2", width)
      .attr("y1", y)
      .attr("y2", y)
      .attr("stroke", "grey");
    svg.append("text")
      .text(y - startY + "ms")
        .attr("text-anchor", "middle")
        .attr("x", 10)
        .attr("y", y - 10) 
        .attr("font-family", "sans-serif")
        .attr("font-size", "11px")
        .attr("fill", "black");
  }

  function DrawTimeLine(){
    // x1,x2 - отрезок по x
    for (var i = 0; i < size; i++) {
      for (var j = 0; j < countOfThreads; j++) {
        var x = startX + (kThreadDistance*countOfThreads + rankDistance)*i + kThreadDistance*j;
        svg.append("line")
          .attr("x1", x)
          .attr("x2", x)
          .attr("y1", startY)
          .attr("y2", maxTime)
          .attr("stroke", "black");
        svg.append("rect")
          .attr("x", x)
          .attr("y", startY - 10)
          .attr("width", 20)
          .attr("height", 12)
          .style("fill", "white");
        var text = GetThreadName(j);
        svg.append("text")
          .text(text)
          .attr("text-anchor", "middle")
          .attr("x", x)
          .attr("y", startY) 
          .attr("font-family", "sans-serif")
          .attr("font-size", "11px")
          .attr("fill", "black");
      }
    }
  }

  function GetXShift(thread) {
    switch(thread) {
      // Worker
      case 0: {
        return kThreadDistance*4;
      } break;
      // Dispatcher
      case 1: {
        return kThreadDistance*3;
      } break;
      // Old Dispatcher
      case 2:{
        return kThreadDistance*3;
      } break;
      // MapController
      case 3: {
        return kThreadDistance*2;
      } break;
      // OldMapController
      case 4: {
         return kThreadDistance*2;
      } break;
      // Server
      case 5: {
        return kThreadDistance*1;
      } break;
      // Main
      default: {
        return kThreadDistance*0;
      } break;
    }
  }

  function GetThreadName(thread) {
    switch(thread) {
      // Worker
      case 4: {
        return "worker";
      } break;
      case 3: {
        return "dispatcher";
      } break;
      case 2:{
        return "MC";
      } break;
      case 1: {
        return "server";
      } break;
      case 0: {
         return "main";
      } break;
      default: {
        return "";
      } break;
    }
  }
 
  function DrawBlocks(array, color) {
    for (var i = 0; i < array.length; i++) {
        //console.log(array.length);
      for (var j = 0; j < array[i].length; j++) {
        var border = "black";
        var l = array[i][j].length;
        //console.log(array[i][j]);
        var x = startX - 2 + (kThreadDistance*countOfThreads + rankDistance)*i;
        var y = startY + array[i][j][l-3];
        var w = 5;
        var h = array[i][j][l-1];
        if (h == 0) {
          h = 1;
          y -= h;
          border = color;
        }
       
        x += GetXShift(array[i][j][0]);
        
        svg.append("rect")
          .attr("x", x)
          .attr("y", y)
          .attr("width", w)
          .attr("height", h)
          .style("fill", color)
          .style("stroke", border);

        if (h > 100) {
          svg.append("rect")
          .attr("x", x)
          .attr("y", y + h/2 - 10)
          .attr("width", w)
          .attr("height", 12)
          .style("fill", "white");
          svg.append("text")
          .text(h + " ms")
          .attr("text-anchor", "middle")
          .attr("x", x+2*w)
          .attr("y", y+h/2) 
          .attr("font-family", "sans-serif")
          .attr("font-size", "11px")
          .attr("fill", "black");
        }
      }
    }
  }
  var wrongSendThreads = [7,8,9];
  var wrongRecvThreads = [7, 8, 9]

  function CheckSendThread(thread) {
    for (var i = 0; i < wrongSendThreads.length; i++)
      if (thread == wrongSendThreads[i]) return false;
    return true;
  }

  function CheckRecvThread(thread) {
    for (var i = 0; i < wrongRecvThreads.length; i++)
      if (thread == wrongRecvThreads[i]) return false;
    return true;
  }

  function AppendArrows (sendMessages, recvMessages) {
     for (var i = 0; i < sendMessages.length; i++) {
      for (var j = 0; j < sendMessages[i].length; j++) {
        var sendMessage = sendMessages[i][j];
        var l = sendMessage.length;
        var k = sendMessage[3];      
        var isFinded = false;
        if (CheckSendThread(sendMessage[0])) {
          for (var z = 0; z < recvMessages[k].length && !isFinded; z++) {
            var recvMessage = recvMessages[k][z];
            if (recvMessage[1] == sendMessage[1] && recvMessage[2] == sendMessage[2]
              && recvMessage[l-2] > sendMessage[l-3] ) {
              isFinded = true;          
              if (CheckRecvThread(recvMessage[0])) {
                messages.push({
                  sender: i,
                  reciever: k,
                  tag: sendMessage[1],
                  sendThread: sendMessage[0],
                  sendTime: sendMessage[l-3],
                  recvThread: recvMessage[0],
                  recvTime: recvMessage[l-2]
                });
                break;
              }
            }
          }
        }
      }
    }
  }

  var arrowColors = ["lightcoral", "rosybrown", "grey", "pink", "tomato", "darkorange"]
  function DrawArrows(arrows) {
    for (var i = 0; i < arrows.length; i++) {
      var arrow = arrows[i];
      var x1 = startX + (kThreadDistance*countOfThreads + rankDistance)*arrow.sender 
        + GetXShift(arrow.sendThread);
      var x2 = startX + (kThreadDistance*countOfThreads + rankDistance)*arrow.reciever 
        + GetXShift(arrow.recvThread);
      var y1 = startY + arrow.sendTime;
      var y2 = startY + arrow.recvTime;

      if (arrow.sender < arrow.reciever) {
        x1 += 5;
      } else if (arrow.sender > arrow.reciever) {
        x2 += 5;
      }
      var idColor = arrow.sendThread;
      var color = arrowColors[idColor];
      svg.append("line")
        .attr("x1", x1)
        .attr("x2", x2)
        .attr("y1", y1)
        .attr("y2", y2)
        .attr("stroke", color);
      var shiftX = 10, shiftY = 2;
      
      if (arrow.sender < arrow.reciever) {
        x1 = x2 - shiftX;
      } else if (arrow.sender > arrow.reciever) {
        x1 = x2 + shiftX;
      }

      y1 = y2 - shiftY;
      svg.append("line")
        .attr("x1", x1)
        .attr("x2", x2)
        .attr("y1", y1)
        .attr("y2", y2)
        .attr("stroke", color);

      y1 = y2 + shiftY;
      svg.append("line")
        .attr("x1", x1)
        .attr("x2", x2)
        .attr("y1", y1)
        .attr("y2", y2)
        .attr("stroke", color);      
     }
  }

  function GetMessageData(filePath, result, i, color, isSend) {
    d3.csv(filePath + i + ".txt" , function(d) { 
      result.push([]);
      var k = 0;
      for(var j = 0; j < d.length; j++) {  
       if (+d[j].thread < 7) {
          result[i].push([]);
          result[i][k].push(
            +d[j].thread,
            +d[j].tag,
            +d[j].sender,
            +d[j].reciever,
            +d[j].begin,
            +d[j].end,
            +d[j].lifeSpan
          );
          k++;
        }
      }
      i++;
      if (i < size) {
        GetMessageData(filePath, result, i, color, isSend);
      } else if (i == size) {
        if (isSend) {
          GetMessageData("http://localhost:8000/source/recv/recv", recv, 0, "grey", false);
        } else {
          AppendArrows(send,recv);
          DrawArrows(messages);
          DrawTimeLine();
          DrawBlocks(result, color);
          DrawBlocks(send, "green");
          filePath = "http://localhost:8000/source/barrier/barrier";
          GetData(filePath, barrier, 0, "red");
          filePath = "http://localhost:8000/source/wait/wait";
          GetData(filePath, wait, 0, "pink");
          filePath = "http://localhost:8000/source/allReduce/allReduce";
          GetData(filePath, reduce, 0, "teal");
          filePath = "http://localhost:8000/source/dup/dup";
          GetData(filePath, dup, 0, "purple");
        }
      }
    })
  }

  function GetData(filePath, result, i, color) {
    d3.csv(filePath + i + ".txt", function(d) {      
      result.push([]);
      for(var j = 0; j < d.length; j++) {   
        result[i].push([]);
        result[i][j] = [ 
          +d[j].thread,
          +d[j].begin,
          +d[j].end,
          +d[j].lifeSpan
        ];
      }
      i++;
      if (i < size) {
        GetData(filePath, result, i, color);
      } else if (i == size) {
        DrawBlocks(result, color);
      }
    })
  }
  //for (var i = 0; i < size; i++) {
    var filePath = "http://localhost:8000/source/send/send";
    GetMessageData(filePath, send, 0, "green", true);
    
})