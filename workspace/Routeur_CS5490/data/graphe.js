"use strict";

var plot;
var data_graphe = [[], [], [], [], [], []];  // Time, P_rms, U_rms, Temp, Temp, Temp

var plot_energy;
var energy_graphe = [[], [], []];  // Time, Econso, Esurplus

//let energy_graphe = [
//  [1546300800, 1546387200, 1546473600, 1546560000, 1546646400, 1546732800],    // x-values (timestamps)
//  [        35,         71, 50, 120, 45, 150],    // y-values (series 1)
//  [        90,         15, 30, 5, 12, 24],    // y-values (series 2)
//];

// ***************************
// IHM functions
// ***************************

function doClearGraphe() {
  for (var i = 0; i < data_graphe.length; i++) {
    data_graphe[i] = [];
  }
  plot.setData(data_graphe);
}

function doExportGraphe() {

  var data_csv = "";
  for (var i = 0; i < data_graphe[0].length; i++) {
    // Convert unix datetime to readable date
    let dateObject = new Date(data_graphe[0][i] * 1000);
    let str = dateObject.toLocaleString();
    for (var j = 1; j < data_graphe.length; j++) {
      str += "\t" + (data_graphe[j][i]).toString().replace(".", ",");
    }
    data_csv += str + "\r\n";
  }

  var blob = new Blob([data_csv],
          { type: "text/plain; charset=utf-8" });

  // Save the file with FileSaver.js
  saveAs(blob, "graphe.csv");
}

function refreshGraph(target) {
  if (target === "data")
  {
    doClearGraphe();
    ESP_Request("Update_CSV", ["/data.csv", target]);
  }
  else
    if (target === "energy")
    {
 //     doClearGraphe();
      ESP_Request("Update_CSV", ["/energy.csv", target]);
    }
}

function updateGraph(data, target, append) {
  if (target === "data")
  {
    if (append)
      for (var i = 0; i < data_graphe.length; i++) {
        data_graphe[i].push(data[i]);
      }
    else
      data_graphe = data;

    plot.setData(data_graphe);
  }
  else
    if (target === "energy")
    {
      energy_graphe = data;
      plot_energy.setData(energy_graphe);
    }
}

// ***************************
// Plot function
// ***************************

// Parse un fichier csv et renvoie un tableau de nb_var tableaux [[], ..., []]
function parseCSV(string, nb_var) {
  var array = new Array(nb_var);
  for (let i = 0; i < nb_var; i++)
    array[i] = new Array();

  var lines = string.split("\r\n");
  for (var i = 0; i < lines.length; i++) {
    if (lines[i] != "")
    {
      var data = lines[i].split("\t");
      var data_size = data.length;
      for (let j = 0; j < nb_var; j++) {
        if (j < data_size)
          array[j].push(parseFloat(data[j]));
        else
          array[j].push(0);
      }
    }
  }
  return array;
}

// column-highlights the hovered x index
function columnHighlightPlugin({ className, style = {backgroundColor: "rgba(51,204,255,0.3)"} } = {}) {
    let underEl, overEl, highlightEl, currIdx;

    function init(u) {
      underEl = u.under; // u.root.querySelector(".u-under");
      overEl = u.over; // u.root.querySelector(".u-over");

      highlightEl = document.createElement("div");

      className && highlightEl.classList.add(className);

      uPlot.assign(highlightEl.style, {
          pointerEvents: "none",
          display: "none",
          position: "absolute",
          left: 0,
          top: 0,
          height: "100%",
          ...style
      });

      underEl.appendChild(highlightEl);

      // show/hide highlight on enter/exit
      overEl.addEventListener("mouseenter", () => {highlightEl.style.display = null;});
      overEl.addEventListener("mouseleave", () => {highlightEl.style.display = "none";});
    }
  
    function update(u) {
      if ((u.cursor.idx !== null) && (currIdx !== u.cursor.idx)) {
        currIdx = u.cursor.idx;

        let [iMin, iMax] = u.series[0].idxs;

        const dx = iMax - iMin;
        if (dx != 0) {
          const width = (u.bbox.width / dx) / devicePixelRatio;
          const xVal  = u.scales.x.distr == 2 ? currIdx : u.data[0][currIdx];
          const left  = u.valToPos(xVal, "x") - width / 2;

          highlightEl.style.transform = "translateX(" + Math.round(left) + "px)";
          highlightEl.style.width = Math.round(width) + "px";
        }
      }
    }  

    return {
        opts: (u, opts) => {
            uPlot.assign(opts, {
                cursor: {
                    x: true,
                    y: true,
                }
            });
        },
        hooks: {
            init: init,
            setCursor: update,
        }
    };
}

function wheelZoomPlugin(opts) {
	// 缩放倍率
    let factor = opts.factor || 0.75;

	return {
		hooks: {
			ready: u => {
				// 获取绘制曲线区域
				let over = u.over;
				let rect = over.getBoundingClientRect();

				// 给坐标轴添加鼠标滚动缩放和拖拽事件
				for (let key in u.axes) {
					// 获取坐标轴css区域
					let axis = u.axes[key]._el;
					// 获取坐标轴面积等参数
					let a_rect = axis.getBoundingClientRect();
					// 添加参数
					axis._params = [key, a_rect];
					// 坐标轴区域监听鼠标按下
					axis.addEventListener("mousedown", e => {
						// 鼠标中键按下
						if (e.button == 1) {
							e.preventDefault();
							// 获取传递的参数
							let key = e.target._params[0];
							// 获取对应坐标轴的ID
							let scale_key = u.axes[key].scale;
							// 取得坐标轴
							let scale = u.scales[scale_key]
							let umin = scale.min;
							let umax = scale.max;
							// 记录坐标轴和像素的比例
							let scale_scale = u.posToVal(0, scale_key) - u.posToVal(1, scale_key);
							// 记录鼠标起始位置
							let left0 = e.clientX;
							let top0 = e.clientY;

							// 当坐标轴按下鼠标中键且移动时
							function onmove(e) {
								e.preventDefault();

								// 记录移动的位置
								let left1 = e.clientX;
								let top1 = e.clientY;
								// 计算X Y偏移量
								let dt = 0;
								if (scale.ori === 0) { // 为X轴
									let dx = (left1 - left0);
									dt = dx;
								} else if (scale.ori === 1) {
									let dy = top1 - top0;
									dt = dy;
								} else { return; }

								// 记录其他坐标的范围，防止被设为自动
								let urange = []
								for (let key in u.scales) {
									urange[key] = [u.scales[key].min, u.scales[key].max];
								}

								// 开始缩放，设置坐标轴最值
								u.setScale(scale_key, {
									min: umin + dt * scale_scale,
									max: umax + dt * scale_scale,
								});

								// 复原其他坐标轴
								for (let key in urange) {
									if (key !== scale_key) {
										u.setScale(key, {
											min: urange[key][0],
											max: urange[key][1],
										});
									}
								}

							}
							// 鼠标中键松开事件
							function onup(e) {
								// 移除事件回调
								document.removeEventListener("mousemove", onmove);
								document.removeEventListener("mouseup", onup);
							}
							// 监听鼠标移动、松开事件
							document.addEventListener("mousemove", onmove);
							document.addEventListener("mouseup", onup);
						}
					});

					// 坐标轴区域添加鼠标滚轮事件
					axis.addEventListener("wheel", e => {
						e.preventDefault();

						// 获取参数
						let key = e.target._params[0];
						let scale_key = u.axes[key].scale;

						// 对范围进行缩放，以中点为基点
						let range = u.scales[scale_key].max - u.scales[scale_key].min;
						let val = (u.scales[scale_key].max + u.scales[scale_key].min) / 2;
						// 缩放的比例
						range = e.deltaY < 0 ? range * factor : range / factor;

						let nMin = val - range / 2;
						let nMax = val + range / 2;

						let urange = []
						for (let key in u.scales) {
							urange[key] = [u.scales[key].min, u.scales[key].max];
						}
						// 设置坐标轴范围
						u.batch(() => {
							u.setScale(scale_key, {
								min: nMin,
								max: nMax,
							});
						});
						// 恢复其他坐标轴
						for (let key in urange) {
							if (key !== scale_key) {
								u.setScale(key, {
									min: urange[key][0],
									max: urange[key][1],
								});
							}
						}
					});
				}


				// 曲线区域添加拖放操作
				over.addEventListener("mousedown", e => {
					// 鼠标中键按下
					if (e.button == 1) {
						//	plot.style.cursor = "move";
						e.preventDefault();
						// 记录按下位置
						let left0 = e.clientX;
						let top0 = e.clientY;
						// 获取各个Y轴的最大值、最小值、坐标轴比例（应该是比例？）和轴类型（0是x轴，1是y轴）加入字典
						let scY = new Array()
						for (var key in u.scales) {
							scY[key] = [u.scales[key].min, u.scales[key].max, u.posToVal(1, key) - u.posToVal(0, key), u.scales[key].ori];
						}
						// let scXMin0 = u.scales.x.min;
						// let scXMax0 = u.scales.x.max;

						// let xUnitsPerPx = u.posToVal(1, 'x') - u.posToVal(0, 'x');
						// 当按下鼠标中键且移动时
						function onmove(e) {
							e.preventDefault();
							// 记录移动的位置
							let left1 = e.clientX;
							let top1 = e.clientY;
							// 计算X Y偏移量
							let dx = (left1 - left0);
							let dy = top1 - top0;

							// 开始移动，设置所有坐标轴
							for (var key in u.scales) {
								if (scY[key] !== null) {
									let dt = 0;
									if (scY[key][3] === 0) {
										// x轴
										dt = dx;
									}
									else if (scY[key][3] === 1) {
										// y 轴
										dt = dy;
									} else {
										continue;
									}
									u.setScale(key, {
										min: scY[key][0] - dt * scY[key][2],
										max: scY[key][1] - dt * scY[key][2],
									});
								}
							}
						}
						// 鼠标中键松开事件
						function onup(e) {
							// 移除事件回调
							document.removeEventListener("mousemove", onmove);
							document.removeEventListener("mouseup", onup);
						}
						// 监听鼠标移动、松开事件
						document.addEventListener("mousemove", onmove);
						document.addEventListener("mouseup", onup);
					}
				});

				// 曲线区域添加滚轮缩放操作
				over.addEventListener("wheel", e => {
					e.preventDefault();

					// 获取鼠标位置
					let { left, top } = u.cursor;

					let oRange = new Array();
					// 遍历各个刻度范围
					for (var key in u.scales) {
						let Val = 0;
						// 判断坐标轴
						if (u.scales[key].ori === 0) {
							Val = u.posToVal(left, key);
						} else if (u.scales[key].ori === 1) {
							Val = u.posToVal(top, key)
						} else { continue; }
						// 获取原始坐标轴范围和缩放后的范围
						let range = u.scales[key].max - u.scales[key].min;
						let nRange = e.deltaY < 0 ? range * factor : range / factor;
						// 计算鼠标所在位置的相对比例并计算新范围
						let Pct = (Val - u.scales[key].min) / range;
						let nMin = Val - Pct * nRange;
						let nMax = nMin + nRange;
						oRange[key] = [nMin, nMax];
					}
					u.batch(() => {
						for (var key in oRange) {
							u.setScale(key, {
								min: oRange[key][0],
								max: oRange[key][1],
							});
						}
					});
				});
			}
		}
	};
}

const fmtDate = uPlot.fmtDate("{DD}/{MM}/{YYYY} {H}:{mm}");
const tzDate = ts => uPlot.tzDate(new Date(ts * 1e3));
function fmtValue(val, prec, unit) {
  return Number(val).toFixed(prec) + unit;
}

// La graduation pour l'axe des abscisses (temps)
const timeAxis = {
  space: 40,
  incrs: [
     // minute divisors (# of secs)
     1,
     5,
     10,
     15,
     30,
     // hour divisors
     60,
     60 * 5,
     60 * 10,
     60 * 15,
     60 * 30,
     // day divisors
     3600,
     3600 * 5,
     3600 * 10,
     3600 * 15,
     // month divisors
     3600 * 24,
     3600 * 24 * 5,
     3600 * 24 * 10
  // ...
  ],
  // [0]:   minimum num secs in found axis split (tick incr)
  // [1]:   default tick format
  // [2-7]: rollover tick formats
  // [8]:   mode: 0: replace [1] -> [2-7], 1: concat [1] + [2-7]
  values: [
  // tick incr          default           year                             month    day                        hour     min                sec       mode
    [3600 * 24 * 365,   "{YYYY}",         null,                            null,    null,                      null,    null,              null,        1],
    [3600 * 24 * 28,    "{MMM}",          "\n{YYYY}",                      null,    null,                      null,    null,              null,        1],
    [3600 * 24,         "{D}/{M}",        "\n{YYYY}",                      null,    null,                      null,    null,              null,        1],
    [3600,              "{H}:00",         "\n{D}/{M}/{YY}",                null,    "\n{D}/{M}",               null,    null,              null,        1],
    [60,                "{H}:{mm}",       "\n{D}/{M}/{YY}",                null,    "\n{D}/{M}",               null,    null,              null,        1],
    [1,                 ":{ss}",          "\n{D}/{M}/{YY} {H}:{mm}",       null,    "\n{D}/{M} {H}:{mm}",      null,    "\n{H}:{mm}",      null,        1],
    [0.001,             ":{ss}.{fff}",    "\n{D}/{M}/{YY} {H}:{mm}",       null,    "\n{D}/{M} {H}:{mm}",      null,    "\n{H}:{mm}",      null,        1],
  ],
  //      label: "Date"
  //  splits:
}

function getDataPlotOptions() {
  return  {
    id: "chart",
    class: "data-chart",
  //  width: 800,
  //  height: 300,
    plugins: [
      columnHighlightPlugin(),
      wheelZoomPlugin({factor: 0.75})
    ],
    ...getSize(),
    title: "Puissance, Tension et Température",

  //  tzDate: ts => uPlot.tzDate(new Date(ts * 1e3)),
    tzDate: ts => uPlot.tzDate(new Date(ts * 1e3), 'Europe/Paris'),

    // Options des séries
    series: [
      {
        label: "Date ",
        value: (u, ts) => fmtDate(tzDate(ts))
      },
      {
        show: true,
        spanGaps: false,

        // in-legend display
        label: "Puissance ",
        value: (u, v) => fmtValue(v, 2, " W"),

        // series style
        stroke: "blue",
        scale: "P",
        width: 1,
        fill: "rgba(0, 0, 255, 0.2)",
        dash: [10, 5],
      },
      {
        label: "Tension ",
        stroke: "green",
        scale: "T",
        width: 2,
        value: (u, v) => fmtValue(v, 2, " V"),
      },
      {
        label: "Temp Cirrus ",
        stroke: "red",
        scale: "Q",
        width: 2,
        value: (u, v) => fmtValue(v, 2, "°C"),
      },
      {
        label: "Temp interne ",
        stroke: "yellow",
        scale: "Q",
        width: 2,
        value: (u, v) => fmtValue(v, 2, "°C"),
      },
      {
        label: "Temp externe ",
        stroke: "magenta",
        scale: "Q",
        width: 2,
        value: (u, v) => fmtValue(v, 2, "°C"),
      }
    ],

    // Options des axes. Le premier est l'axe des abscisses (temps).
    axes: [
      timeAxis,

      {
        scale: "P",
        values: (u, vals) => vals.map(v => v.toFixed(1)),
        label: "Watt"
      },
      {
        scale: "T",
        values: (u, vals) => vals.map(v => v.toFixed(2)),
        label: "Volt",
        side: 1,
        grid: {show: false},
      },
      {
        scale: "Q",
        values: (u, vals) => vals.map(v => v.toFixed(1)),
        label: "°C",
        side: 1,
      }
    ],
    legend : {
      show: true,
      live: false
    },
    tooltip : {
      show: true
    }
  };
}

function getEnergyPlotOptions() {
  return  {
    id: "chart_energy",
    class: "energy-chart",
  //  width: 800,
  //  height: 300,
    plugins: [
//      columnHighlightPlugin(),
      wheelZoomPlugin({factor: 0.75})
    ],
    ...getSize(),
    title: "Energie conso et surplus",

  //  tzDate: ts => uPlot.tzDate(new Date(ts * 1e3)),
    tzDate: ts => uPlot.tzDate(new Date(ts * 1e3), 'Europe/Paris'),

    // Options des séries
    series: [
      {
        label: "Date ",
        value: (u, ts) => fmtDate(tzDate(ts))
      },
      {
        label: "E conso ",
        stroke: "green",
        fill: "#33BB55FF",
        paths: uPlot.paths.bars({align: -1, size: [1, 10], gap: 0}),

//        paths: uPlot.paths.bars({
//          align: -1,
//          size: [1, 100],
//          gap: 20,
//            disp: {
//                stroke: {
//                    unit: 3,
////                    values: (u, seriesIdx) => u.data[2].map(v =>
////                        v == 0 ? "#33BB55" :
////                        v == 1 ? "#F79420" :
////                                 "#BB1133"
////                    ),
//                },
////                fill: {
////                    unit: 3,
////                    values: (u, seriesIdx) => u.data[2].map(v =>
////                        v == 0 ? "#33BB55A0" :
////                        v == 1 ? "#F79420A0" :
////                                 "#BB1133A0"
////                    ),
////                }
//            }
//        }),

        scale: "E",
        width: 1,
        value: (u, v) => fmtValue(v, 2, " Wh"),
      },
      {
        label: "E surplus ",
        stroke: "red",
        fill: "#F79420FF",
        paths: uPlot.paths.bars({align: 1, size: [1, 10], gap: 0}),
        scale: "E",
        width: 1,
        value: (u, v) => fmtValue(v, 2, " Wh"),
      }
    ],

    // Options des axes. Le premier est l'axe des abscisses (temps).
    axes: [
      timeAxis,

      {
        scale: "E",
        values: (u, vals) => vals.map(v => v.toFixed(2)),
        label: "Wh",
        grid: {show: true},
      }
    ],
    legend : {
      show: true,
      live: false
    },
    tooltip : {
      show: true
    }
  };
}

// ***************************
// Initialization
// ***************************

// To resize the plot
function getSize() {
  return {
      "width": window.innerWidth - 50,
      "height": window.innerHeight - 250
  };
}

function initializeGraphe() {

  plot = new uPlot(getDataPlotOptions(), data_graphe, document.getElementById("chart"));
  plot_energy = new uPlot(getEnergyPlotOptions(), energy_graphe, document.getElementById("chart_energy"));

  window.addEventListener("resize", e => {
      plot.setSize(getSize());
      plot_energy.setSize(getSize());
  });
}

