import plotly
import plotly.graph_objs as go
import numpy as np

import const

# x_detectors = [-4 , 0 , 4]
# y_detectors = [4 , -4 , 4]
xypairs = list(const.ANCHORS.values())
x_detectors = [xy[0] for xy in xypairs]
y_detectors = [xy[1] for xy in xypairs]
sz = [20,20,20]
colors = [0.1,0.1,0.1]

def plot_heatmap(pos=[[5,5,5]]):

    # print(const.positions.keys())
    # print(pos)
    if(len(pos)==0):
        return
    return

    points_distances = pos
    trace0 = go.Scatter(
    	x=x_detectors,
        y=y_detectors,
        mode='markers',
        marker={'size': sz,
                'color': colors,
                'opacity': 0.6,
                'colorscale': 'Viridis'
                }
    )
    data = [trace0]

    shapes = []
    for point_distances in points_distances:
    	for i,radii in enumerate(point_distances):
    		# print(i,radii)
    		x_i = x_detectors[i]
    		y_i = y_detectors[i]
    		shapes.append({
    			'type': 'circle',
    			'xref': 'x',
    			'yref': 'y',
    			'x0': x_i - radii,
    			'y0': y_i - radii,
    			'x1': x_i + radii,
    			'y1': y_i + radii,
    			'line': {
    				'color': 'rgba(50, 171, 96, 1)',
    	        	},
    			})

    layout = {
        'xaxis': {
            'range': [-500, 1000],
            'zeroline': False,
        },
        'yaxis': {
            'range': [-500, 1000]
        },
        'width': 800,
        'height': 800,
        'shapes': shapes,
    }
    fig = {
        'data': data,
        'layout': layout,
    }

    plotly.offline.plot(fig, auto_open=True)




# fig.add_scatter(x=x,
#                 y=y,
#                 mode='markers',
#                 marker={'size': sz,
#                         'color': colors,
#                         'opacity': 0.6,
#                         'colorscale': 'Viridis'
#                        });

